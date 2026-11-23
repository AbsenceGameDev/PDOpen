/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#include "SaveEditor/RTSOSaveEditorStyle.h"

#include "Misc/CommandLine.h"
#include "Styling/StarshipCoreStyle.h"

#include "Settings/EditorStyleSettings.h"

#include "SlateOptMacros.h"
#include "Styling/SlateStyleMacros.h"

#if (WITH_EDITOR || (IS_PROGRAM && PLATFORM_DESKTOP))
	#include "PlatformInfo.h"
#endif
#include "Styling/ToolBarStyle.h"
#include "Styling/SegmentedControlStyle.h"
#include "Styling/StyleColors.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"

#define ICON_FONT(...) FSlateFontInfo(RootToContentDir("Fonts/PermanentMarker", TEXT(".ttf")), __VA_ARGS__)

#define LOCTEXT_NAMESPACE "SRTSOSaveEditorStyle"

FName FRTSOSaveEditorStyle::StyleSetName = TEXT("EditorStyle");

void FRTSOSaveEditorStyle::Initialize()
{
	LLM_SCOPE_BYNAME(TEXT("FRTSOSaveEditorStyle"));

	// The core style must be initialized before the editor style
	FSlateApplication::InitializeCoreStyle();

	const FString ThemesSubDir = TEXT("Slate/Themes");

#if ALLOW_THEMES
	USlateThemeManager::Get().ApplyTheme(USlateThemeManager::Get().GetCurrentTheme().Id);
	//UStyleColorTable::Get().SaveCurrentThemeAs(UStyleColorTable::Get().GetCurrentTheme().Filename);
#endif
}

void FRTSOSaveEditorStyle::Shutdown()
{
}

const FName& FRTSOSaveEditorStyle::GetStyleSetName()
{
	return FRTSOSaveEditorStyle::StyleSetName;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

/* FRTSOSaveEditorStyle static initialization
 *****************************************************************************/

void FRTSOSaveEditorStyle::FStyle::SetColor(const TSharedRef< FLinearColor >& Source, const FLinearColor& Value)
{
	Source->R = Value.R;
	Source->G = Value.G;
	Source->B = Value.B;
	Source->A = Value.A;
}

/* FRTSOSaveEditorStyle interface
 *****************************************************************************/

FRTSOSaveEditorStyle::FStyle::FStyle()
	: FSlateStyleSet(FRTSOSaveEditorStyle::StyleSetName)

	// Note, these sizes are in Slate Units.
	// Slate Units do NOT have to map to pixels.
	, Icon7x16(7.0f, 16.0f)
	, Icon8x4(8.0f, 4.0f)
	, Icon16x4(16.0f, 4.0f)
	, Icon8x8(8.0f, 8.0f)
	, Icon10x10(10.0f, 10.0f)
	, Icon12x12(12.0f, 12.0f)
	, Icon12x16(12.0f, 16.0f)
	, Icon14x14(14.0f, 14.0f)
	, Icon16x16(16.0f, 16.0f)
	, Icon16x20(16.0f, 20.0f)
	, Icon20x20(20.0f, 20.0f)
	, Icon22x22(22.0f, 22.0f)
	, Icon24x24(24.0f, 24.0f)
	, Icon25x25(25.0f, 25.0f)
	, Icon32x32(32.0f, 32.0f)
	, Icon40x40(40.0f, 40.0f)
	, Icon48x48(48.0f, 48.0f)
	, Icon64x64(64.0f, 64.0f)
	, Icon36x24(36.0f, 24.0f)
	, Icon128x128(128.0f, 128.0f)

	// These are the colors that are updated by the user style customizations
	, SelectionColor_Subdued_LinearRef(MakeShareable(new FLinearColor(0.807f, 0.596f, 0.388f)))
	, HighlightColor_LinearRef( MakeShareable( new FLinearColor(0.068f, 0.068f, 0.068f) ) ) 
	, WindowHighlightColor_LinearRef(MakeShareable(new FLinearColor(0,0,0,0)))


	// These are the Slate colors which reference those above; these are the colors to put into the style
	, SelectionColor_Subdued( SelectionColor_Subdued_LinearRef )
	, HighlightColor( HighlightColor_LinearRef )
	, WindowHighlightColor(WindowHighlightColor_LinearRef)
	, InheritedFromBlueprintTextColor(FLinearColor(0.25f, 0.5f, 1.0f))
{
}

FRTSOSaveEditorStyle::FStyle::~FStyle()
{
	// // @note @todo set up callback 
	// if (Settings.IsValid())
	// {
	// 	Settings->OnSettingChanged().Remove(SettingChangedHandler);
	// }
}


static void AuditDuplicatedCoreStyles(const ISlateStyle& EditorStyle)
{
	const ISlateStyle& CoreStyle = FRTSOSaveEditorStyle::GetCoreStyle();
	TSet<FName> CoreStyleKeys = CoreStyle.GetStyleKeys();

	TSet<FName> EditorStyleKeys = EditorStyle.GetStyleKeys();

	TSet<FName> DuplicatedNames = CoreStyleKeys.Intersect(EditorStyleKeys);

	DuplicatedNames.Sort(FNameLexicalLess());
	for (FName& Name : DuplicatedNames)
	{
		UE_LOG(LogSlate, Log, TEXT("%s"), *Name.ToString());
	}
}

void FRTSOSaveEditorStyle::FStyle::Initialize()
{
	SetParentStyleName("CoreStyle");

	SetContentRoot( FPaths::EngineContentDir() / TEXT("Editor/Slate") );
	SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	SetupSaveEditorStyles();
	SetupGeneralIcons();
	
	AuditDuplicatedCoreStyles(*this);
	
	// // @note @todo set up callback 
	// if (Settings.IsValid())
	// {
	// 	SettingChangedHandler = Settings->OnSettingChanged().AddRaw(this, &FRTSOSaveEditorStyle::FStyle::SettingsChanged);
	// }

}

void FRTSOSaveEditorStyle::FStyle::SetupSaveEditorStyles()
{
	// // Normal Text
	// {
	// 	Set( "RichTextBlock.TextHighlight", FTextBlockStyle(NormalText)
	// 		.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f ) ) );
	// 	Set( "RichTextBlock.Bold", FTextBlockStyle(NormalText)
	// 		.SetFont( DEFAULT_FONT("Bold", FRTSOSaveEditorStyle::RegularTextSize )) );
	// 	Set( "RichTextBlock.BoldHighlight", FTextBlockStyle(NormalText)
	// 		.SetFont( DEFAULT_FONT("Bold", FRTSOSaveEditorStyle::RegularTextSize ))
	// 		.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f ) ) );
	// 	Set("RichTextBlock.Italic", FTextBlockStyle(NormalText)
	// 		.SetFont(DEFAULT_FONT("Italic", FRTSOSaveEditorStyle::RegularTextSize)));
	// 	Set("RichTextBlock.ItalicHighlight", FTextBlockStyle(NormalText)
	// 		.SetFont(DEFAULT_FONT("Italic", FRTSOSaveEditorStyle::RegularTextSize))
	// 		.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f)));
	//
	// 	Set( "TextBlock.HighlightShape",  new BOX_BRUSH( "Common/TextBlockHighlightShape", FMargin(3.f/8.f) ));
	// 	Set( "TextBlock.HighlighColor", FLinearColor( 0.02f, 0.3f, 0.0f ) );
	//
	// 	Set("TextBlock.ShadowedText", FTextBlockStyle(NormalText)
	// 		.SetShadowOffset(FVector2D(1.0f, 1.0f))
	// 		.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f)));
	//
	// 	Set("TextBlock.ShadowedTextWarning", FTextBlockStyle(NormalText)
	// 		.SetColorAndOpacity(FStyleColors::Warning)
	// 		.SetShadowOffset(FVector2D(1.0f, 1.0f))
	// 		.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f)));
	//
	// 	Set("NormalText.Subdued", FTextBlockStyle(NormalText)
	// 		.SetColorAndOpacity(FSlateColor::UseSubduedForeground()));
	//
	// 	Set("NormalText.Important", FTextBlockStyle(NormalText)
	// 		.SetFont(DEFAULT_FONT("Bold", FRTSOSaveEditorStyle::RegularTextSize))
	// 		.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
	// 		.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
	// 		.SetShadowOffset(FVector2D(1, 1))
	// 		.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)));
	//
	// 	Set("SmallText.Subdued", FTextBlockStyle(NormalText)
	// 		.SetFont(DEFAULT_FONT("Regular", FRTSOSaveEditorStyle::SmallTextSize))
	// 		.SetColorAndOpacity(FSlateColor::UseSubduedForeground()));
	//
	// 	Set("TinyText", FTextBlockStyle(NormalText)
	// 		.SetFont(DEFAULT_FONT("Regular", FRTSOSaveEditorStyle::SmallTextSize)));
	//
	// 	Set("TinyText.Subdued", FTextBlockStyle(NormalText)
	// 		.SetFont(DEFAULT_FONT("Regular", FRTSOSaveEditorStyle::SmallTextSize))
	// 		.SetColorAndOpacity(FSlateColor::UseSubduedForeground()));
	//
	// 	Set("LargeText", FTextBlockStyle(NormalText)
	// 		.SetFont(DEFAULT_FONT("Bold", 11))
	// 		.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
	// 		.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
	// 		.SetShadowOffset(FVector2D(1, 1))
	// 		.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)));
	// }
	// 	
	
	// // Example - General Icons
	// 	{
	// 		Set("RTSOSaveEditor.VisibleIcon16x", new CORE_IMAGE_BRUSH_SVG("Starship/Common/visible", Icon16x16));
	// 		Set("RTSOSaveEditor.VisibleHighlightIcon16x", new CORE_IMAGE_BRUSH_SVG("Starship/Common/visible", Icon16x16));
	// 		Set("RTSOSaveEditor.NotVisibleIcon16x", new CORE_IMAGE_BRUSH_SVG("Starship/Common/hidden", Icon16x16));
	// 		Set("RTSOSaveEditor.NotVisibleHighlightIcon16x", new CORE_IMAGE_BRUSH_SVG("Starship/Common/hidden", Icon16x16));
	// 	}
}

void FRTSOSaveEditorStyle::FStyle::SetupGeneralIcons()
{
	Set("Plus", new IMAGE_BRUSH("Icons/PlusSymbol_12x", Icon12x12));
	Set("Cross", new IMAGE_BRUSH("Icons/Cross_12x", Icon12x12));
	Set("ArrowUp", new IMAGE_BRUSH("Icons/ArrowUp_12x", Icon12x12));
	Set("ArrowDown", new IMAGE_BRUSH("Icons/ArrowDown_12x", Icon12x12));
	Set("Icons.TextEditor", new IMAGE_BRUSH_SVG("Starship/Common/TextEditor_16", Icon16x16));
}

#undef DEFAULT_FONT
#undef ICON_FONT
#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


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

