// Fill out your copyright notice in the Description page of Project Settings.

// Utility file for including OpenVDB headers with necessary workaround macros

THIRD_PARTY_INCLUDES_START

#pragma warning(disable : 4146)
#pragma warning(disable : 4582)
#pragma warning(disable : 4583)

#pragma push_macro("TEXT")
#pragma push_macro("check")
#pragma push_macro("CONSTEXPR")

#undef TEXT
#undef check
#undef CONSTEXPR

#define NOMINMAX
