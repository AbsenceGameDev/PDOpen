/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once


#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MassEntityTypes.h"
#include "Containers/Deque.h"
#include "PDRTSCommon.h"
#include "HAL/UnrealMemory.h"
#include "Misc/ScopeLock.h"
#include "Misc/ScopeRWLock.h"

#include "PDRTSSharedOctree.generated.h"


/** @brief Entity Octree cell data */
struct PDRTSBASE_API FPDEntityOctreeCell
{
	/** @brief Tracked Entity */
	FMassEntityHandle EntityHandle;
	
	/** @brief Cell Bounds */
	FBoxCenterAndExtent Bounds;

	/** @brief Cell ID */
	TSharedPtr<FOctreeElementId2> SharedCellID;
};

/** @brief Boilerplate TOctree2 functional structure */
struct FPDEntityOctreeSemantics 
{
	/** @brief anonymous enum, won't collide. Tells the octree how many elements we can store per leaf node  */
	enum { MaxElementsPerLeaf = 128 };
	/** @brief anonymous enum, won't collide. Tells the octree our minimum limit for inclusive elements per node */
	enum { MinInclusiveElementsPerNode = 7 };
	/** @brief anonymous enum, won't collide. Tells the octree how deep our nodes can get */
	enum { MaxNodeDepth = 12 };

	/** @brief Leaf element allocator */
	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	/** @brief Returns the bounding box of the give cell */
	FORCEINLINE static const FBoxCenterAndExtent& GetBoundingBox(const FPDEntityOctreeCell& Element)
	{
		return Element.Bounds;
	}

	/** @brief Octree Cell comparison */
	FORCEINLINE static bool AreElementsEqual(const FPDEntityOctreeCell& A, const FPDEntityOctreeCell& B)
	{
		return A.EntityHandle == B.EntityHandle;
	}

	/** @brief Octree Cell ID assignment */
	FORCEINLINE static void SetElementId(const FPDEntityOctreeCell& Element, FOctreeElementId2 Id)
	{
		*Element.SharedCellID = Id;
	};
};

/** @brief Boilerplate Entity namespace, defines an octree sub-class. */
namespace PD::Mass
{
	class FOctreeBase
	{
	public:
		FOctreeBase() {};
		virtual ~FOctreeBase() = default;
		
		/** @brief Calls SetupOctreeWithNewWorld(TemporaryWorldCache) */
		virtual void SetupOctree();

		/** @brief
		 * - Registers world into 'WorldsWithOctrees' if not already registered.
		 * - Calls 'DispatchOctreeGeneration' if world is not valid or not initialized
		 * - Generates octree in and adds NewWorld to WorldsWithOctrees to track it
		 */
		virtual bool SetupOctreeWithNewWorld(UWorld* NewWorld);

		UWorld* TemporaryWorldCache = nullptr;

		/** @brief Tracking worlds that has been setup with this WorldOctree.  */
		TMap<void*, bool> WorldsWithOctrees{};

		FSimpleDelegate DispatchOctreeGeneration; 
	};
	
	namespace Entity
	{
		/** Entity octree declaration */
		class Octree final
			: public FOctreeBase
			  , public TOctree2<FPDEntityOctreeCell, FPDEntityOctreeSemantics>
		{
		public:
			Octree() = default;
			Octree(const FVector& InOrigin, FVector::FReal InExtent)
				: TOctree2(InOrigin, InExtent) {}
			
			virtual bool SetupOctreeWithNewWorld(UWorld* NewWorld) override; // final inferred from class being final
		};
		
	}

	// namespace Actor
	// {
	// 	/** Actor octree declaration  */		
	// 	class Octree final
	// 		: public FOctreeBase
	// 		  , public TOctree2<FPDActorOctreeCell, FPDActorOctreeSemantics>
	// 	{
	// 	public:
	// 		Octree() = default;
	// 		Octree(const FVector& InOrigin, FVector::FReal InExtent)
	// 			: TOctree2(InOrigin, InExtent) {}
			
	// 		virtual bool SetupOctreeWithNewWorld(UWorld* NewWorld) override; // final inferred from class being final
	// 	};		
	// }	
}

/** @brief Spatial Entity Query compound,
 * @note For our custom spatial queries against entities we use this compound to store a copy of the handle, location and ownerID */
USTRUCT()
struct FLEntityCompound
{
	GENERATED_BODY()

	bool operator==(const FLEntityCompound& Other) const { return this->EntityHandle == Other.EntityHandle; }
	
	/** @brief Queried entity */
	UPROPERTY()
	FMassEntityHandle EntityHandle;
	/** @brief Location when queried */
	UPROPERTY()
	FVector Location;
	/** @brief Entities OwnerID when queried */
	UPROPERTY()
	int32 OwnerID;
};

/** @brief Helper struct to encode our entity data as pixel data */
struct FPDRTSPerPixelStorageHelper
{
	/** @brief Encode our entity data as pixel data */
	FORCEINLINE static FLinearColor ConstructData(FVector Location, uint16_t Entity16WayRotation, uint8_t EntityFlags, uint16_t TeamColourId)
	{
 		const uint32_t ConstructedAlphaChannel = Entity16WayRotation | uint32_t(EntityFlags) >> 7 | uint32_t(TeamColourId) >> 15;
    	return FLinearColor(
    		Location.X, 
    		Location.Y,
    		Location.Z, 
    		*reinterpret_cast<const float*>(&ConstructedAlphaChannel));
	}

	/** @brief Encode some entity data as alpha channel data */
	FORCEINLINE static uint32 ConstructData(uint16_t Entity16WayRotation, uint8_t EntityFlags, uint8_t TeamColourId)
	{
 		return Entity16WayRotation | uint32_t(EntityFlags) >> 7 | uint32_t(TeamColourId) >> 15;
	}


	/** @brief Deconstruct pixel data into entity data  */
	FORCEINLINE static void DeconstructData(const FLinearColor& InData, FVector& OutLocation, uint16_t& OutEntity16WayRotation, uint8_t& OutEntityFlags, uint16_t& OutTeamColourId)
	{
		OutLocation.X = InData.R;
		OutLocation.Y = InData.G;
		OutLocation.Z = InData.B;

		const uint32 AlphaAsBits = *reinterpret_cast<const uint32*>(&InData.A);
		OutEntity16WayRotation = AlphaAsBits > (32-4);
		OutEntityFlags = (static_cast<uint32>(AlphaAsBits > (32-15)) < 7);
		OutTeamColourId = AlphaAsBits < 15;
	}	
};

	
/** @brief Query base shape */
template<typename TDataType>
struct TPDQueryBase 
{
	/** @brief Shape selector */
	int32 Shape = 0; // 0= Box, 1 Sphere, 2 Ellipsoid? etc..
	/** @brief  Center location of our shape, used when doing intersection checks */
	UE::Math::TVector<TDataType> Location = UE::Math::TVector<TDataType>::ZeroVector;
};	

/** @brief Octree user query helpers */
using FPDOctreeUserQuery = struct QPDUserQuery_t
{
	QPDUserQuery_t() = default;
	explicit QPDUserQuery_t(AActor* CallingActor) : CallingUser(CallingActor) { }
	
	/** @brief Query Box shape definition */
	struct QBox    : public TPDQueryBase<double> { UE::Math::TVector<double> QuerySizes = UE::Math::TVector<double>::ZeroVector; };
	struct QSphere : public TPDQueryBase<double> { double QueryRadius = 0.0; };
	struct FBoundsSimple{FVector Min = PD::Constants::INVALID_WORLD_LOC; FVector Size = PD::Constants::INVALID_WORLD_LOC;};
	/** @brief Query Sphere shape definition */
	// @note Alignment is same and they share the same initial memory layout,
	// @note I'll get away with accessing the inner shape from either or to then resolve the rest to access proper memory space
	union QShapeSelector
	{
		QShapeSelector() { AsBox = QBox{}; };
		
		QBox AsBox;
		QSphere AsSphere;
		void Set(const QBox& InAsBox)
		{
			AsBox = InAsBox;
			AsBox.Shape = 0;
		};
		void Set(const QSphere& InAsSphere)
		{
			AsSphere = InAsSphere;
			AsSphere.Shape = 1;
		};
		TPDQueryBase<double>* Get() const
		{
			const void* This = this;
			const TPDQueryBase<double>* PtrHack = static_cast<const TPDQueryBase<double>*>(This);
			return const_cast<TPDQueryBase<double>*>(PtrHack);
		};
	};

	/** @brief Buffer type selector used creating and/or modifying user query buffers.  */
	enum class EBufferType : uint8 
	{
		INVALID = 0,
		EntityCompound,
		PackedData
	};
	
	
	/** @brief Makes a user query shape based on input params */
	static QBox MakeUserQuery(const UE::Math::TVector<double>& Location = UE::Math::TVector<double>::ZeroVector, const FVector& QuerySizes = UE::Math::TVector<double>::OneVector)
	{
		return QPDUserQuery_t::QBox{0,Location, QuerySizes};
	}
	/** @brief Makes a user query shape based on input params */
	static QSphere MakeUserQuery(const UE::Math::TVector<double>& Location = UE::Math::TVector<double>::ZeroVector, const double Radius = 0.0)
	{
		return QPDUserQuery_t::QSphere{1,Location, Radius};
	}
	/** @return true - if the given point within the box shape, convert shape to TBox<double> and call IsPointWithinQuery */
	static bool IsPointWithinQuery(const UE::Math::TVector<double>& Point, const QBox& Box)
	{
		const UE::Math::TBox<double> InnerBox{{Box.Location - Box.QuerySizes},{Box.Location + Box.QuerySizes}};
		return IsPointWithinQuery(Point, InnerBox);
	}
	/** @return true - if the given point within the sphere shape, convert shape to TSphere<double> and call IsPointWithinQuery  */
	static bool IsPointWithinQuery(const UE::Math::TVector<double>& Point, const QSphere& Sphere)
	{
		const UE::Math::TSphere<double> InnerSphere{Sphere.Location, Sphere.QueryRadius};
		return IsPointWithinQuery(Point, InnerSphere);
	}			
	
	/** @return true - if the given point within the TBox */
	static bool IsPointWithinQuery(const UE::Math::TVector<double>& Point, const UE::Math::TBox<double>& Box)
	{
		return Box.IsInsideOrOn(Point);
	}
	/** @return true - if the given point within the TSphere */
	static bool IsPointWithinQuery(const UE::Math::TVector<double>& Point, const UE::Math::TSphere<double>& Sphere)
	{
		return Sphere.IsInside(Point);
	}
	
	/** @return true - if the given point within the TBox */
	static FBoundsSimple GetQueryBounds(const UE::Math::TBox<double>& Box)
	{
		FVector HalfSize = Box.GetSize() * 0.5;
		FVector Min = Box.GetCenter() - HalfSize;
		return FBoundsSimple{Min, Box.GetSize()};
	}
	/** @return true - if the given point within the TSphere */
	static FBoundsSimple GetQueryBounds(const UE::Math::TSphere<double>& Sphere)
	{
		float Val = (3 * Sphere.GetVolume()) / (4 * UE_PI); 
		float Radius = FMath::Pow(Val, 1.0/3.0);

		FVector HalfSize = {Radius, Radius, Radius};
		FVector Min = Sphere.Center - HalfSize;
		return FBoundsSimple{Min, HalfSize * 2};
	}

	/** @return true - if the given point within the box shape, convert shape to TBox<double> and call IsPointWithinQuery */
	static FBoundsSimple GetQueryBounds(const QBox& Box)
	{
		const UE::Math::TBox<double> InnerBox{{Box.Location - Box.QuerySizes},{Box.Location + Box.QuerySizes}};
		return GetQueryBounds(InnerBox);
	}
	/** @return true - if the given point within the sphere shape, convert shape to TSphere<double> and call IsPointWithinQuery  */
	static FBoundsSimple GetQueryBounds(const QSphere& Sphere)
	{
		const UE::Math::TSphere<double> InnerSphere{Sphere.Location, Sphere.QueryRadius};
		return GetQueryBounds(InnerSphere);
	}			



	/** @brief  Creates a new Query Shape (QBox) entry */
	void CreateNewQueryEntry(const int32 Key, EBufferType BufferType, const QBox& BoxData)
	{
		QShapeSelector InData;
		InData.Set(BoxData);

		{
			FScopeLock Lock(&ArchetypeCS);
			QueryArchetypes.FindOrAdd(Key, InData);
		}

		{
			FWriteScopeLock Lock(BufferRWLock);
			CurrentBuffer.FindOrAdd(Key);
			BufferTypeMapping.FindOrAdd(Key, BufferType);		
		}
	}
	/** @brief  Creates a new Query Shape (QSphere) entry */
	void CreateNewQueryEntry(const int32 Key, EBufferType BufferType, const QSphere& SphereData)
	{
		QShapeSelector InData;
		InData.Set(SphereData);

		{
			FScopeLock Lock(&ArchetypeCS);
			QueryArchetypes.FindOrAdd(Key, InData);
		}

		{
			FWriteScopeLock Lock(BufferRWLock);
			CurrentBuffer.FindOrAdd(Key);
			BufferTypeMapping.FindOrAdd(Key, BufferType);
		}		
	}
	/** @brief Updates a keyed query position */
	void UpdateQueryPosition(const int32 Key, const FVector& NewPos)
	{
		FScopeLock Lock(&ArchetypeCS);
		QueryArchetypes.FindOrAdd(Key).Get()->Location = NewPos;
	}

	template<EPDQueryGroups TKey>
	FVector GetLatestQueryPosition() const
	{
		FScopeLock Lock(&ArchetypeCS);
		QShapeSelector* ObjectAsBase = QueryArchetypes.Find(TKey);
		if(ObjectAsBase == nullptr){return PD::Constants::INVALID_WORLD_LOC;}

		return ObjectAsBase->Get()->Location;
	}

	template<EPDQueryGroups TKey>
	FBoundsSimple GetLatestQueryBounds() const
	{
		const QShapeSelector* ObjectAsBase = QueryArchetypes.Find(TKey);
		if(ObjectAsBase == nullptr){return FBoundsSimple{};}

		const TPDQueryBase<double>* BasePtr = ObjectAsBase->Get();
		switch (ObjectAsBase->Get()->Shape)
		{
		case 0: // Box
			return GetQueryBounds(*static_cast<const QBox*>(BasePtr));
		case 1: // Sphere
			return GetQueryBounds(*static_cast<const QSphere*>(BasePtr));
		default:
			break;
		}

		return FBoundsSimple{};
	}	
	
	/** @brief Performs an "overlap" query on a given position, optionally stores an ownerID and entity handle if point is within shape.
	 * @note Stores the result in a RWLocked buffer  */
	void UpdateQueryOverlapBuffer(const int32 Key, const FVector& ComparePos, const FMassEntityHandle OptionalEntityHandle, const int32 OptionalID)
	{
		bool bIsPointWithinQuery = false;
		QShapeSelector ObjectAsBase = QueryArchetypes.FindOrAdd(Key);
		TPDQueryBase<double>* BasePtr = ObjectAsBase.Get();
		switch (ObjectAsBase.Get()->Shape)
		{
		case 0: // Box
			bIsPointWithinQuery = IsPointWithinQuery(ComparePos, *static_cast<QBox*>(BasePtr));
			break;
		case 1: // Sphere
			bIsPointWithinQuery = IsPointWithinQuery(ComparePos, *static_cast<QSphere*>(BasePtr));
			break;
		default:
			break;
		}
		
		{
			FWriteScopeLock Lock(BufferRWLock);
			if (bIsPointWithinQuery)
			{
				FLEntityCompound EntityCompound{OptionalEntityHandle, ComparePos, OptionalID};
				CurrentBuffer.FindOrAdd(Key).Emplace(EntityCompound);
			}
			// Clear key if it still persists for some reason, this fricking data-race condition is fixed now but damn it is not pretty
			else if (CurrentBuffer.Contains(Key))
			{
				TArray<FLEntityCompound>& EntityArray = *CurrentBuffer.Find(Key);
				const FLEntityCompound EntityCompound{OptionalEntityHandle, ComparePos, OptionalID};
				EntityArray.Remove(EntityCompound);
			}
		}		
	}

	/** @brief Performs an "overlap" query on a given position, with the option to store encoded entity pixel if point is in the query .
	 * @note Stores the result in a RWLocked buffer  */	
	void UpdateQueryOverlapBuffer(const int32 Key, const FVector& ComparePos, const FLinearColor& MinimapEncodedData)
	{
		bool bIsPointWithinQuery = false;
		QShapeSelector ObjectAsBase = QueryArchetypes.FindOrAdd(Key);
		TPDQueryBase<double>* BasePtr = ObjectAsBase.Get();
		switch (ObjectAsBase.Get()->Shape)
		{
		case 0: // Box
			bIsPointWithinQuery = IsPointWithinQuery(ComparePos, *static_cast<QBox*>(BasePtr));
			break;
		case 1: // Sphere
			bIsPointWithinQuery = IsPointWithinQuery(ComparePos, *static_cast<QSphere*>(BasePtr));
			break;
		default:
			break;
		}
		
		{
			FWriteScopeLock Lock(BufferRWLock);
			if (bIsPointWithinQuery)
			{
				CurrentPackedDataBuffer.FindOrAdd(Key).Emplace(MinimapEncodedData);
			}
			// Clear key if it still persists for some reason, this fricking data-race condition is fixed now but damn it is not pretty
			else if (CurrentPackedDataBuffer.Contains(Key))
			{
				TArray<FLinearColor>& PackedDataArray = *CurrentPackedDataBuffer.Find(Key);
				PackedDataArray.Remove(MinimapEncodedData);
			}
		}
	}	
	
	/** @brief Removes a buffer entry and query archetypes, via key */
	void RemoveQueryData(const int32 Key)
	{
		FWriteScopeLock Lock(BufferRWLock);

		QueryArchetypes.Remove(Key);
		CurrentBuffer.Remove(Key);
		BufferTypeMapping.Remove(Key);
	}
	/** @brief Clears buffer entry and resets its query archetypes, via key.
	 * @note does not remove any key entries */
	void ClearQueryDataAndSettings(int32 Key)
	{
		FWriteScopeLock Lock(BufferRWLock);

		QShapeSelector ObjectAsBase = QueryArchetypes.FindOrAdd(Key);
		TPDQueryBase<double>* BasePtr = ObjectAsBase.Get();
		switch (ObjectAsBase.Get()->Shape)
		{
		case 0: // Box
			{
				QBox& QueryEntry = *static_cast<QBox*>(BasePtr);
				QueryEntry.Location  = FVector::ZeroVector;
				QueryEntry.QuerySizes = FVector::OneVector;
			}
			break;
		case 1: // Sphere
			{
				QSphere& QueryEntry = *static_cast<QSphere*>(BasePtr);
				QueryEntry.Location    = FVector::ZeroVector;
				QueryEntry.QueryRadius = 0.0;
			}
			break;
		default:
			break;
		}
		
		ClearQueryBuffer(Key);
	}
	/** @brief Clears buffer entry, via key.
	 * @note does not remove any key entry */
	void ClearQueryBuffer(int32 Key)
	{
		FWriteScopeLock Lock(BufferRWLock);
		
		EBufferType* BufferTypePtr = BufferTypeMapping.Find(Key);
		if (UNLIKELY(BufferTypePtr == nullptr))
		{
			ClearBuffer(Key, CurrentBuffer);
			ClearBuffer(Key, CurrentPackedDataBuffer);
		}
		switch (*BufferTypePtr)
		{
		case EBufferType::EntityCompound:
			ClearBuffer(Key, CurrentBuffer);
			break;
		case EBufferType::PackedData:
			ClearBuffer(Key, CurrentPackedDataBuffer);
			break;
			
		case EBufferType::INVALID:
		default:
			break;
		}
	}

	/** @brief  Calling user unused, @todo perhaps time to deprecate and/or remove s */
	FORCEINLINE void SetCallingUser(AActor* Caller) { CallingUser = Caller;}

	using BufferSelectorConds = struct BufferSelectorConds_t
	{
		bool bIsMinimapGroup;
		bool bIsInvalid;
		bool bIsOther;
	};
	
	/** @brief Buffer selector conditions used to clean up some std::conditionals  */
	template<EPDQueryGroups TKey = EPDQueryGroups::INVALID_QUERY_GROUP>
	static constexpr BufferSelectorConds GetBufferSelectorConds()
	{
			constexpr bool bIsMinimapGroup = TKey == EPDQueryGroups::QUERY_GROUP_MINIMAP;
			constexpr bool bIsInvalid = TKey == EPDQueryGroups::INVALID_QUERY_GROUP;
			constexpr bool bIsOther = !bIsMinimapGroup && !bIsInvalid;
			constexpr BufferSelectorConds Conds{bIsMinimapGroup, bIsInvalid, bIsOther};
			return Conds;
	}	
#define DefineBufferSelector \
		using TBufferSelector = std::conditional_t<Conds.bIsMinimapGroup, TArray<FLinearColor>, std::conditional_t<Conds.bIsOther, TArray<FLEntityCompound>, void>>;

#define IsBufferValidReadPos ((TPos == EBufferReadPos::INDEX && BufferPtr->IsValidIndex(BufferOffset)) || BufferPtr->IsValidIndex(BufferPtr->Num() - 1 - BufferOffset))
	template<EPDQueryGroups TKey, EBufferReadPos TPos>
	FQueryResult_LocAndId ReadQueryBufferAtPosition(int32 BufferOffset) const
	{
		FReadScopeLock Lock(BufferRWLock);

		constexpr BufferSelectorConds Conds = GetBufferSelectorConds<TKey>();
		DefineBufferSelector
		
		const TBufferSelector* BufferPtr;
		if constexpr (Conds.bIsMinimapGroup) 
		{ 
			BufferPtr = CurrentPackedDataBuffer.Find(TKey);
			if (BufferPtr && IsBufferValidReadPos)
			{
				auto BufferData = TPos == EBufferReadPos::INDEX ? (*BufferPtr)[BufferOffset] : (*BufferPtr)[BufferPtr->Num() - 1 - BufferOffset];

				FVector OutLocation; 
				uint16 OutEntity16WayRotation;
				uint8 OutEntityFlags; 
				uint16_t OutTeamColourId;
				FPDRTSPerPixelStorageHelper::DeconstructData(BufferData, OutLocation, OutEntity16WayRotation, OutEntityFlags, OutTeamColourId);
				return FQueryResult_LocAndId{OutLocation, OutTeamColourId, FMassEntityHandle{INDEX_NONE}};
			}
		}
		if constexpr (Conds.bIsOther) 
		{ 
			BufferPtr = CurrentBuffer.Find(TKey); 
			if (BufferPtr && IsBufferValidReadPos)
			{
				const FLEntityCompound& EntityCompound = TPos == EBufferReadPos::INDEX ? (*BufferPtr)[BufferOffset] : (*BufferPtr)[BufferPtr->Num() - 1 - BufferOffset];
				return FQueryResult_LocAndId{EntityCompound.Location, static_cast<int16>(EntityCompound.OwnerID), EntityCompound.EntityHandle};
			}
		}
		return FQueryResult_LocAndId{};
	}

	template<EPDQueryGroups TKey>
	FQueryResult_LocAndId ReadQueryBufferFromEnd(int32 BufferOffset) const
	{
		return ReadQueryBufferAtPosition<TKey, EBufferReadPos::END>(BufferOffset);
	}

	template<EPDQueryGroups TKey>
	FQueryResult_LocAndId ReadQueryBufferFromIndex(int32 BufferIndex) const
	{
		return ReadQueryBufferAtPosition<TKey, EBufferReadPos::INDEX>(BufferIndex);
	}	

	template<EPDQueryGroups TKey>
	void MemCpyQueryBuffer(std::conditional_t<TKey == EPDQueryGroups::QUERY_GROUP_MINIMAP, TArray<FLinearColor>&, TArray<FLEntityCompound>&> Target)
	{
		constexpr BufferSelectorConds Conds = GetBufferSelectorConds<TKey>();
		DefineBufferSelector
		
		EBufferType* BufferTypePtr = BufferTypeMapping.Find(TKey);
		if (Conds.bIsInvalid || UNLIKELY(BufferTypePtr == nullptr))
		{
			return;
		}

		{ //Lock
			FWriteScopeLock Lock(BufferRWLock);
			
			TBufferSelector* BufferPtr;
			if constexpr (Conds.bIsMinimapGroup) { BufferPtr = CurrentPackedDataBuffer.Find(TKey);}
			if constexpr (Conds.bIsOther) { BufferPtr = CurrentBuffer.Find(TKey); }
			
			if constexpr (Conds.bIsMinimapGroup || Conds.bIsOther)
			{
				if (BufferPtr == nullptr) {return;}
				Target.Reserve(BufferPtr->Num());
				Target = MoveTemp(*BufferPtr);
			}
		}
	}

private:
	template<typename TBuffer>
	void ClearBuffer(int32 Key, TBuffer& Buffer)
	{
		if (Buffer.Contains(Key)) { Buffer.FindRef(Key).Empty(); }
		else { Buffer.Add(Key).Empty();}
	}

public:
	mutable FRWLock BufferRWLock;
	mutable FCriticalSection ArchetypeCS;
private:

	/** @brief  Query shape archetypes */
	TMap<int32, QShapeSelector> QueryArchetypes;
	/** @brief  User Query buffers */
	TMap<int32, EBufferType> BufferTypeMapping;
	TMap<int32, TArray<FLEntityCompound>> CurrentBuffer;
	TMap<int32, TArray<FLinearColor>> CurrentPackedDataBuffer;
	/** @brief  User that will be calling this query structure */
	AActor* CallingUser = nullptr;
};

/** @brief Entity octree ID, used for observer tracking if IDs has been invalidated */
USTRUCT()
struct PDRTSBASE_API FPDOctreeFragment : public FMassFragment
{
	GENERATED_BODY()

	/** @brief CellID of a given octree cell using this fragment*/
	TSharedPtr<FOctreeElementId2> CellID;
};

/** @brief Entity octree grid cell tag */
USTRUCT(BlueprintType)
struct PDRTSBASE_API FPDInOctreeGridTag : public FMassTag
{
	GENERATED_BODY()
};

/** @brief Entity  query ('line-trace') tag. Used for entities we want to be processed with line intersection octree searches */
USTRUCT(BlueprintType)
struct PDRTSBASE_API FPDOctreeQueryTag : public FMassTag
{
	GENERATED_BODY()
};

namespace MassSample::Signals
{
	/** @brief Signal name for receiving a hit */
	static const FName OnReceiveHit = FName("OnReceiveHit");
	/** @brief Signal name for performing a hit */
	static const FName OnEntityHitOther = FName("OnEntityHitOther");
}



/**
Business Source License 1.1

Parameters

Licensor:             Ario Amin (@ Permafrost Development)
Licensed Work:        RTSOpen (version 0.1.0, Source available on github)
                      The Licensed Work is (c) 2026 Ario Amin (@ Permafrost Development)
Additional Use Grant: You may make free use of the Licensed Work in a commercial product or service provided these additional conditions as met; 
                      1. Must give attributions to the original author of the Licensed Work, in 'Credits' if that is applicable.
                      2. The Licensed Work must be 'Compiled' before being redistributed.
                      3. The Licensed Work 'Source' may be linked but may not be packaged into the product or service being sold
                      4. Must not be resold or repackaged or redistributed as another product, is only allowed to be used within a commercial or non-commercial game project.
                      5. Teams whose 'Total Finances' exceed $100,000 USD for the most recent 12-month period must contact the owner for a custom license or buy the framework from a marketplace it has been made available on.

                      "Credits" indicate a scrolling screen with attributions. This is usually in a products end-state

                      "Total Finances" means the largest of your aggregate gross revenues, entire budget, or funding (no matter the source).
                      "Package" means the collection of files distributed by the Licensor, and derivatives of that collection
                      and/or of those files..   

                      "Source" form means the source code, documentation source, and configuration files for the Package, usually in human-readable format.

                      "Compiled" form means the compiled bytecode, object code, binary, or any other
                      form resulting from mechanical transformation or translation of the Source form.

Change Date:          2030-04-26

Change License:       Apache License, Version 2.0

For information about alternative licensing arrangements for the Software,
please visit: https://permadev.se/

Notice

The Business Source License (this document, or the “License”) is not an Open
Source license. However, the Licensed Work will eventually be made available
under an Open Source License, as stated in this License.

License text copyright (c) 2017 MariaDB Corporation Ab, All Rights Reserved.
“Business Source License” is a trademark of MariaDB Corporation Ab.

-----------------------------------------------------------------------------

Business Source License 1.1

Terms

The Licensor hereby grants you the right to copy, modify, create derivative
works, redistribute, and make non-production use of the Licensed Work. The
Licensor may make an Additional Use Grant, above, permitting limited
production use.

Effective on the Change Date, or the fourth anniversary of the first publicly
available distribution of a specific version of the Licensed Work under this
License, whichever comes first, the Licensor hereby grants you rights under
the terms of the Change License, and the rights granted in the paragraph
above terminate.

If your use of the Licensed Work does not comply with the requirements
currently in effect as described in this License, you must purchase a
commercial license from the Licensor, its affiliated entities, or authorized
resellers, or you must refrain from using the Licensed Work.

All copies of the original and modified Licensed Work, and derivative works
of the Licensed Work, are subject to this License. This License applies
separately for each version of the Licensed Work and the Change Date may vary
for each version of the Licensed Work released by Licensor.

You must conspicuously display this License on each original or modified copy
of the Licensed Work. If you receive the Licensed Work in original or
modified form from a third party, the terms and conditions set forth in this
License apply to your use of that work.

Any use of the Licensed Work in violation of this License will automatically
terminate your rights under this License for the current and all other
versions of the Licensed Work.

This License does not grant you any right in any trademark or logo of
Licensor or its affiliates (provided that you may use a trademark or logo of
Licensor as expressly required by this License).

TO THE EXTENT PERMITTED BY APPLICABLE LAW, THE LICENSED WORK IS PROVIDED ON
AN “AS IS” BASIS. LICENSOR HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS,
EXPRESS OR IMPLIED, INCLUDING (WITHOUT LIMITATION) WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT, AND
TITLE.

MariaDB hereby grants you permission to use this License’s text to license
your works, and to refer to it using the trademark “Business Source License”,
as long as you comply with the Covenants of Licensor below.

Covenants of Licensor

In consideration of the right to use this License’s text and the “Business
Source License” name and trademark, Licensor covenants to MariaDB, and to all
other recipients of the licensed work to be provided by Licensor:

1. To specify as the Change License the GPL Version 2.0 or any later version,
   or a license that is compatible with GPL Version 2.0 or a later version,
   where “compatible” means that software provided under the Change License can
   be included in a program with software provided under GPL Version 2.0 or a
   later version. Licensor may specify additional Change Licenses without
   limitation.

2. To either: (a) specify an additional grant of rights to use that does not
   impose any additional restriction on the right granted in this License, as
   the Additional Use Grant; or (b) insert the text “None”.

3. To specify a Change Date.

4. Not to modify this License in any other way.
 **/