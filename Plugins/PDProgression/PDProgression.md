## PRIO 0 . CREATE PROGRESSION SYSTEM PLUGIN (PSEUDO-CODE/Design ideas)
- Stats (Progression meta-rules)

-- FPDRulesetOperatorStruct
- Properties:
- OpType           : (Example: 'EAddition', 'ESubtraction', 'EMultiplication', 'EDivision', 'EPower')
- OpTarget         : (Example: EThis, EOther, EStatic)
- OpFallbackValue  : (Example: If EThis or EOther, it will be ignored, if EStatic it will be the value used)
- PotentialInnerOp : FDataTableRowHandle RulesetHandle // Row Type: FPDRulesetOperatorStruct

-- FPDRulesetEvaluator
- Eval (const TArray<FPDRulesetOperatorStruct>& Ruleset,
const TValueType& PotentialSelfValue,
const TValueType& PotentialOtherValue,
const TValueType& PotentialStaticValue,
)
{
const int32 RuleLimits = Ruleset.Num()
TMap<int32, TArray<int32> /**/> IndexOrderOfOperations;

			// Tally Top-level operations
			for (int32 RuleIdx = 0;  RuleIdx < RuleLimits; RuleIdx++)
			{
				const FPDRulesetOperatorStruct& Rule = Ruleset[RuleIdx];
				switch (Rule.OpType)
				{
					case EAddition: case ESubtraction:
						SelectedPrecedence = 0;
						break;
					case EMultiplication: case EDivision:
						SelectedPrecedence = 1;
						break;
					case EPower:
						SelectedPrecedence = 2;
						break;
				}

				TArray<int32>& InnerToplevelOperations =
					IndexOrderOfOperations.Contains(SelectedPrecedence)
					? *IndexOrderOfOperations.Find(SelectedPrecedence)
					: IndexOrderOfOperations.Emplace(SelectedPrecedence)
				InnerToplevelOperations.Emplace(RuleIdx);
			}

			// Process operations
			constexpr int32 MaxPrecedence = 2;

			TArray<int32> OpValues;
			OpValues.SetSize(RuleLimits);

			for (int32 PrecedenceLevel = MaxPrecedence; PrecedenceLevel >= 0; PrecedenceLevel--)
			{
				if (IndexOrderOfOperations.Contains(PrecedenceLevel) == false)
				{
					continue;
				}

				TArray<int32>& InnerToplevelOperations = *IndexOrderOfOperations.Find(PrecedenceLevel)

				// Tally all operation values
				for (const int32 RuleIndex : InnerToplevelOperations)
				{
					const FPDRulesetOperatorStruct& Rule = Ruleset[RuleIndex];

					switch(Rule.OpTarget)
					{
						case EThis:   OpValues[RuleIndex] = PotentialSelfValue; break;
						case EOther:  OpValues[RuleIndex] = PotentialOtherValue; break;
						case EStatic: OpValues[RuleIndex] = PotentialStaticValue; break;
					}
				}

				for (const int32 RuleIndex : InnerToplevelOperations)
				{
					// First index can't apply their operator to a previous rule. so it is ignored from this point
					if (RuleIndex == 0)
					{
						continue;
					}
					const FPDRulesetOperatorStruct& PreviousRule = Ruleset[RuleIndex - 1];
					const FPDRulesetOperatorStruct& CurrentRule = Ruleset[RuleIndex];

					TValueType PreviousOpBaseValue = OpValues[RuleIndex - 1];
					TValueType CurrentOpBaseValue = OpValues[RuleIndex];

					// Expand value of PreviousRule before handling the operator from the CurrentOpBaseValue
					TValueType ExpandedPreviousIndexValue = 0;
					if (PreviousRule.RulesetHandle.IsValidRow()
					{
						PreviousOpBaseValue
						TArray<FPDRulesetOperatorStruct> InnerRulesetRecursion;
						FPDRulesetOperatorStruct* PrevInnerRulesetPtr = PreviousRule.RulesetHandle.GetRow<FPDRulesetOperatorStruct>("");
						if (PrevInnerRulesetPtr != nullptr)
						{
							InnerRulesetRecursion.Emplace(*PrevInnerRulesetPtr);
							FPDRulesetEvaluator::Eval(InnerRulesetRecursion, PotentialSelfValue, PotentialOtherValue, PotentialStaticValue);
						}

					}

					// Expand value of CurrentRule before handling the operator from the CurrentOpBaseValue
					TValueType ExpandedCurrentIndexValue = 0;
					if (CurrentRule.RulesetHandle.IsValidRow()
					{
						CurrentOpBaseValue
						CurrentRule.RulesetHandle.GetRow<FPDRulesetOperatorStruct>()->;

					}

					TValueType OperationResult
					const FPDRulesetOperatorStruct& Rule = Ruleset[RuleIdx];
					switch (Rule.OpType)
					{
						case EAddition:
							OperationResult = ExpandedPreviousIndexValue + ExpandedCurrentIndexValue;
							break;
						case ESubtraction:
							OperationResult = ExpandedPreviousIndexValue - ExpandedCurrentIndexValue;
							break;
						case EMultiplication:
							OperationResult = ExpandedPreviousIndexValue * ExpandedCurrentIndexValue;
							break;
						case EDivision:
							OperationResult = ExpandedPreviousIndexValue / ExpandedCurrentIndexValue;
							break;
						case EPower:
							OperationResult = FMath::Pow(ExpandedPreviousIndexValue, CurrentOpBaseValue);
							break;
					}

				}

				//
				//
				//
				//
				//
				//

				// FDataTableRowHandle RulesetHandle
			}

		}

	-

-- FPDRulesetTableRow
- FGameplayTag RuleSetTagID;
- TArray<FPDRulesetOperatorStruct> RuleSetOperation;
// For example: A Rule-Set TagID may be something like 'Progression.RuleSet.DnD'
// For example: A Rule-Set TagID may be something like 'Progression.RuleSet.ObsidianPoE'
// For example: A Rule-Set TagID may be something like 'Progression.RuleSet.Custom0'
// For example: A Rule-Set TagID may be something like 'Progression.RuleSet.Custom1'

-- FPDAllowedStackingContexts(Having )
- TMap<FGameplayTag (*StackingContextTagID*/, FDataTableRowHandle /* type: FPDRuleSetTableRow> ContextList */ >;

	// For example: A Stacking Context TagID may be something like 'Progression.StackingContext.Gear'

- Stats Overview
    - Enum  EPDProgressionBehaviourType { EClassic, EActionBased }
    - Enum  EPDProgressionType { EActiveEffect, EPassiveEffect, EStat}
    - If Actionbased,
        - Action Event Delegate signature: bool (void* OpaqueActionDataPacket){}
        - run an automated test-level before packaging to ensure an action event has been tied to the stat to increase it.

    - FPDStatsValue
        - TArray<int32> BaseValueRepresentations;
        - int32 BaseDivisor = 1;

    - FPDStatsRow
        - FGameplayTag ProgressionTag;
        - TMap<FGameplayTag /*StatTag*/, FGameplayTag /*RuleSetTag*/> RulesAffectedBy;
        - EPDProgressionBehaviourType BehaviourType;
        - EPDProgressionType ProgressionType;
        - FPDStatsValue ProgressionValueRepresentation;
        - int32 MaxLevel = 1;
        - UCurveFloat ExperienceCurve;

    - FPDStat : public FFastArraySerializerItem
        - FGameplayTag ProgressionTag;
        - int32 CurrentLevel = 0;
        - int32 CurrentExperience;

    - FPDStatList : public FFastArraySerializerItemList
        - TArray<FPDStat> Items;

    - FPDStatMapping
        - int32 Index = 0;
        - FGameplayTag Tag;
        - bool operator ==(const int32 OtherIndex) { return this->Index == OtherIndex; }
        - bool operator ==(const FGameplayTag& OtherTag) { return this->Tag == OtherTag; }

    - Skilltrees
        - Research tree is a skill tree which is applied to some other actor and controlled by the owning actor ID
        -

    - USTRUCT(Blueprintable) struct FPDProgressionClassRow : public FTableRowBase
      {
      UPROPERTY(EditAnywhere, BlueprintReadWrite)
      FGameplayTag Tag

      	UPROPERTY(EditAnywhere, BlueprintReadWrite)
      	TSet<FGameplayTag> DefaultStats;

      	UPROPERTY(EditAnywhere, BlueprintReadWrite)
      	TSet<FGameplayTag> DefaultActiveEffects;

      	UPROPERTY(EditAnywhere, BlueprintReadWrite)
      	TSet<FGameplayTag> DefaultPassiveEffects;

      	UPROPERTY(EditAnywhere, BlueprintReadWrite)
      	TSet<FGameplayTag> GrantedTrees;
      };

    - UPDStatSubsystem
        - TMap<FGamplayTag, FPDProgressionClassRow*> ClassTypes
        - TMap<FGamplayTag, FPDStat> DefaultStatValues

    - FPDStatHandler
        - UFUNCTION(BlueprintNativeEvent) void DefaultFillStatList(const FGameplayTag& ClassTag);
          void DefaultFillStatList_Implementation(cons FGameplayTag& ClassTag)
          {
          FPDProgressionClassRow* ClassRow = UPDStatSubsystem::Get()->GetClassType(ClassTag);
          if (ClassRow == nullptr) { return; }

          for (const FGameplayTag& StatTag : ClassRow->DefaultStats)
          {
          FPDStatList.AddStat(StatTag);
          }

          for (const FGameplayTag& ActiveEffectTag : ClassRow->DefaultActiveEffects)
          {
          FPDStatList.AddActiveEffect(ActiveEffectTag);
          }

          for (const FGameplayTag& PassiveEffectTag : ClassRow->DefaultPassiveEffects)
          {
          FPDStatList.AddPassiveEffect(PassiveEffectTag);
          }
          }

        - UPROPERTY(Replicated) FPDStatList{};
        - TSet<FPDStatMapping> StatTagMappings;

        - IncreaseStatLevel()
        - IncreaseStatExperience()
 Currently the design is that on a server or single player, a save-game is used, on the server it is the source of truth of the world-state for the server and on single-player games it is the same,problem is that several UI's directly depend on the savegame now and in a server game it will not be available for the clients!!! Massive problem. Wil need to have some replication in place and an middleman of sorts