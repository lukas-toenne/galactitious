// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/openvdb.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

struct FASTMULTIPOLESIMULATION_API OpenVDBConvert
{
	static inline FVector Vector(const openvdb::Vec3f& v) { return FVector(v.x(), v.y(), v.z()); }
	static inline openvdb::Vec3f Vector(const FVector& v) { return openvdb::Vec3f(v.X, v.Y, v.Z); }

	static inline FQuat Quat(const openvdb::QuatR& q) { return FQuat(q.x(), q.y(), q.z(), q.w()); }
	static inline openvdb::QuatR Quat(const FQuat& q) { return openvdb::QuatR(q.X, q.Y, q.Z, q.W); }

	static inline FMatrix Matrix4(const openvdb::math::Mat4f& m)
	{
		FMatrix R;
		memcpy(R.M, m.asPointer(), sizeof(float) * 16);
		return R;
	}
	static inline openvdb::math::Mat4f Matrix4(const FMatrix& m)
	{
		openvdb::math::Mat4f R;
		memcpy(R.asPointer(), m.M, sizeof(float) * 16);
		return R;
	}
};
