#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace OpenGlass::Settings
{
	inline constexpr unsigned CatalogVersion = 1;

	enum class Scope : std::uint8_t
	{
		User,
		Machine
	};

	enum class ValueType : std::uint8_t
	{
		Dword,
		String
	};

	enum class AssetRole : std::uint8_t
	{
		None,
		ThemeAtlas,
		Reflection,
		Material
	};

	enum class UpdateImpact : std::uint8_t
	{
		None = 0,
		Colorization = 1,
		Theme = 2,
		RestartRequired = 4
	};

	constexpr UpdateImpact operator|(UpdateImpact left, UpdateImpact right) noexcept
	{
		return static_cast<UpdateImpact>(static_cast<unsigned>(left) | static_cast<unsigned>(right));
	}

	enum class Id : std::uint16_t
	{
		ColorizationColor,
		ColorizationColorOverride,
		ColorizationAfterglow,
		ColorizationAfterglowOverride,
		ColorizationColorBalance,
		ColorizationColorBalanceOverride,
		ColorizationAfterglowBalance,
		ColorizationAfterglowBalanceOverride,
		ColorizationBlurBalance,
		ColorizationBlurBalanceOverride,
		ColorizationColorInactive,
		GlassOpacity,
		GlassOpacityInactive,
		ColorizationColorCaption,
		ColorizationColorCaptionInactive,
		ColorizationColorCaptionMaximized,
		ColorizationColorCaptionInactiveMaximized,
		ColorizationOpaqueBlend,
		ColorizationBaseTransparent,
		ColorizationBaseMaximized,
		ColorizationBaseOpaque,
		ColorizationOpaqueBlendPriority,
		ColorizationOpacity,
		ColorizationOpacityInactive,
		ColorizationOpacityMaximized,
		ColorizationOpacityInactiveMaximized,
		GlassType,
		GlassOverrideAccent,
		CustomThemeReflection,
		ColorizationGlassReflectionIntensity,
		ColorizationGlassReflectionOpacity,
		ColorizationGlassReflectionOpacityInactive,
		ColorizationGlassReflectionOpacityMaximized,
		ColorizationGlassReflectionOpacityInactiveMaximized,
		ColorizationGlassReflectionParallaxIntensity,
		ColorizationGlassReflectionPolicy,
		BlurDeviation,
		BlurOptimization,
		RoundRectRadius,
		CustomThemeMaterial,
		MaterialOpacity,
		UseDirect3DRendering,
		CaptionButtons,
		CenterCaption,
		CaptionTextAliasing,
		TextGlowMode,
		CustomThemeAtlas,
		DisableModernBorders,
		DisableGlassOnBattery,
		DisabledHooks,
		GlassSafetyZoneMode,
		MinMaxButtonGlowId,
		CloseButtonGlowId,
		ToolCloseButtonGlowId,
		Count
	};

	struct Spec
	{
		Id id;
		std::wstring_view name;
		ValueType type;
		AssetRole assetRole;
		UpdateImpact impact;
		bool sensitive;
		unsigned introducedIn{ 1 };
		bool includeInPresetPacks{ true };
	};

	inline constexpr std::array<Spec, static_cast<std::size_t>(Id::Count)> Catalog
	{{
		{ Id::ColorizationColor, L"ColorizationColor", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationColorOverride, L"ColorizationColorOverride", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationAfterglow, L"ColorizationAfterglow", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationAfterglowOverride, L"ColorizationAfterglowOverride", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationColorBalance, L"ColorizationColorBalance", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationColorBalanceOverride, L"ColorizationColorBalanceOverride", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationAfterglowBalance, L"ColorizationAfterglowBalance", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationAfterglowBalanceOverride, L"ColorizationAfterglowBalanceOverride", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationBlurBalance, L"ColorizationBlurBalance", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationBlurBalanceOverride, L"ColorizationBlurBalanceOverride", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationColorInactive, L"ColorizationColorInactive", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::GlassOpacity, L"GlassOpacity", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::GlassOpacityInactive, L"GlassOpacityInactive", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationColorCaption, L"ColorizationColorCaption", ValueType::Dword, AssetRole::None, UpdateImpact::Theme, false },
		{ Id::ColorizationColorCaptionInactive, L"ColorizationColorCaptionInactive", ValueType::Dword, AssetRole::None, UpdateImpact::Theme, false },
		{ Id::ColorizationColorCaptionMaximized, L"ColorizationColorCaptionMaximized", ValueType::Dword, AssetRole::None, UpdateImpact::Theme, false },
		{ Id::ColorizationColorCaptionInactiveMaximized, L"ColorizationColorCaptionInactiveMaximized", ValueType::Dword, AssetRole::None, UpdateImpact::Theme, false },
		{ Id::ColorizationOpaqueBlend, L"ColorizationOpaqueBlend", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationBaseTransparent, L"ColorizationBaseTransparent", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationBaseMaximized, L"ColorizationBaseMaximized", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationBaseOpaque, L"ColorizationBaseOpaque", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationOpaqueBlendPriority, L"ColorizationOpaqueBlendPriority", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationOpacity, L"ColorizationOpacity", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationOpacityInactive, L"ColorizationOpacityInactive", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationOpacityMaximized, L"ColorizationOpacityMaximized", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationOpacityInactiveMaximized, L"ColorizationOpacityInactiveMaximized", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::GlassType, L"GlassType", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::GlassOverrideAccent, L"GlassOverrideAccent", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::CustomThemeReflection, L"CustomThemeReflection", ValueType::String, AssetRole::Reflection, UpdateImpact::Theme, true },
		{ Id::ColorizationGlassReflectionIntensity, L"ColorizationGlassReflectionIntensity", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationGlassReflectionOpacity, L"ColorizationGlassReflectionOpacity", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationGlassReflectionOpacityInactive, L"ColorizationGlassReflectionOpacityInactive", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationGlassReflectionOpacityMaximized, L"ColorizationGlassReflectionOpacityMaximized", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationGlassReflectionOpacityInactiveMaximized, L"ColorizationGlassReflectionOpacityInactiveMaximized", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationGlassReflectionParallaxIntensity, L"ColorizationGlassReflectionParallaxIntensity", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::ColorizationGlassReflectionPolicy, L"ColorizationGlassReflectionPolicy", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::BlurDeviation, L"BlurDeviation", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::BlurOptimization, L"BlurOptimization", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::RoundRectRadius, L"RoundRectRadius", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::CustomThemeMaterial, L"CustomThemeMaterial", ValueType::String, AssetRole::Material, UpdateImpact::Theme, true },
		{ Id::MaterialOpacity, L"MaterialOpacity", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::UseDirect3DRendering, L"UseDirect3DRendering", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::CaptionButtons, L"CaptionButtons", ValueType::Dword, AssetRole::None, UpdateImpact::Theme, false },
		{ Id::CenterCaption, L"CenterCaption", ValueType::Dword, AssetRole::None, UpdateImpact::Theme, false },
		{ Id::CaptionTextAliasing, L"CaptionTextAliasing", ValueType::Dword, AssetRole::None, UpdateImpact::Theme, false },
		{ Id::TextGlowMode, L"TextGlowMode", ValueType::Dword, AssetRole::None, UpdateImpact::Theme, false },
		{ Id::CustomThemeAtlas, L"CustomThemeAtlas", ValueType::String, AssetRole::ThemeAtlas, UpdateImpact::Theme, true },
		{ Id::DisableModernBorders, L"DisableModernBorders", ValueType::Dword, AssetRole::None, UpdateImpact::Theme, false },
		{ Id::DisableGlassOnBattery, L"DisableGlassOnBattery", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, false },
		{ Id::DisabledHooks, L"DisabledHooks", ValueType::Dword, AssetRole::None, UpdateImpact::RestartRequired, true },
		{ Id::GlassSafetyZoneMode, L"GlassSafetyZoneMode", ValueType::Dword, AssetRole::None, UpdateImpact::Colorization, true },
		{ Id::MinMaxButtonGlowId, L"MINMAXBUTTONGLOWid", ValueType::Dword, AssetRole::None, UpdateImpact::RestartRequired, true, 1, false },
		{ Id::CloseButtonGlowId, L"CLOSEBUTTONGLOWid", ValueType::Dword, AssetRole::None, UpdateImpact::RestartRequired, true, 1, false },
		{ Id::ToolCloseButtonGlowId, L"TOOLCLOSEBUTTONGLOWid", ValueType::Dword, AssetRole::None, UpdateImpact::RestartRequired, true, 1, false }
	}};

	consteval bool IsValidCatalog()
	{
		for (std::size_t index = 0; index < Catalog.size(); ++index)
		{
			const auto& spec = Catalog[index];
			if (static_cast<std::size_t>(spec.id) != index || spec.name.empty() || spec.introducedIn == 0 || spec.introducedIn > CatalogVersion) return false;
			if ((spec.type == ValueType::String) != (spec.assetRole != AssetRole::None)) return false;
			if (!spec.includeInPresetPacks && (spec.type != ValueType::Dword || spec.assetRole != AssetRole::None)) return false;
			for (std::size_t other = 0; other < index; ++other)
			{
				if (Catalog[other].name == spec.name) return false;
			}
		}
		return true;
	}

	static_assert(IsValidCatalog());

	[[nodiscard]] constexpr const Spec& Get(Id id) noexcept
	{
		return Catalog[static_cast<std::size_t>(id)];
	}

	static_assert(!Get(Id::DisableGlassOnBattery).sensitive);
	static_assert(Get(Id::GlassOverrideAccent).impact == UpdateImpact::Colorization);
	static_assert(Get(Id::GlassSafetyZoneMode).impact == UpdateImpact::Colorization);
	static_assert(Get(Id::UseDirect3DRendering).impact == UpdateImpact::Colorization);
	static_assert(!Get(Id::UseDirect3DRendering).sensitive);
	static_assert(!Get(Id::MinMaxButtonGlowId).includeInPresetPacks);
	static_assert(!Get(Id::CloseButtonGlowId).includeInPresetPacks);
	static_assert(!Get(Id::ToolCloseButtonGlowId).includeInPresetPacks);

	[[nodiscard]] constexpr bool IsPresetPackSetting(const Spec& spec, unsigned catalogVersion = CatalogVersion) noexcept
	{
		return spec.includeInPresetPacks && spec.introducedIn <= catalogVersion;
	}

	[[nodiscard]] constexpr std::size_t PresetPackSettingCount(unsigned catalogVersion = CatalogVersion) noexcept
	{
		std::size_t count{};
		for (const auto& spec : Catalog)
		{
			if (IsPresetPackSetting(spec, catalogVersion)) ++count;
		}
		return count;
	}

	[[nodiscard]] constexpr const Spec* Find(std::wstring_view name) noexcept
	{
		for (const auto& spec : Catalog)
		{
			if (spec.name == name)
			{
				return &spec;
			}
		}
		return nullptr;
	}

}
