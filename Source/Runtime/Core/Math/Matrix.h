#pragma once
#include <DirectXMath.h>
#include "SimpleMath.h"
namespace DirectX
{
    namespace SimpleMath
    {
        


        struct LMatrix : public XMFLOAT4X4
        {
            static const LMatrix Identity;

            LMatrix() noexcept
                : XMFLOAT4X4(1.f, 0, 0, 0,
                    0, 1.f, 0, 0,
                    0, 0, 1.f, 0,
                    0, 0, 0, 1.f) {}

            constexpr LMatrix(float m00, float m01, float m02, float m03,
                float m10, float m11, float m12, float m13,
                float m20, float m21, float m22, float m23,
                float m30, float m31, float m32, float m33) noexcept
                : XMFLOAT4X4(m00, m01, m02, m03,
                    m10, m11, m12, m13,
                    m20, m21, m22, m23,
                    m30, m31, m32, m33) {}

            explicit LMatrix(const Vector3& r0, const Vector3& r1, const Vector3& r2) noexcept
                : XMFLOAT4X4(r0.x, r0.y, r0.z, 0,
                    r1.x, r1.y, r1.z, 0,
                    r2.x, r2.y, r2.z, 0,
                    0, 0, 0, 1.f) {}

            explicit LMatrix(const Vector4& r0, const Vector4& r1, const Vector4& r2, const Vector4& r3) noexcept
                : XMFLOAT4X4(r0.x, r0.y, r0.z, r0.w,
                    r1.x, r1.y, r1.z, r1.w,
                    r2.x, r2.y, r2.z, r2.w,
                    r3.x, r3.y, r3.z, r3.w) {}

            LMatrix(const XMFLOAT4X4& M) noexcept { memcpy(this, &M, sizeof(XMFLOAT4X4)); }
            LMatrix Transpose() const noexcept;
            Matrix Invert() const noexcept;

            operator XMMATRIX() const noexcept { return XMLoadFloat4x4(this); }

            static LMatrix CreateFromAxisAngle(const Vector3& axis, float angle) noexcept;
            static LMatrix CreateScale(const Vector3& scales) noexcept;
            static LMatrix CreateTranslation(const Vector3& position) noexcept;
            static LMatrix CreateOrthographicOffCenterLH(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane) noexcept;
            static LMatrix CreateMatrixLookAtLH(const Vector3& EyePos, const Vector3& TargetPos, const Vector3& UpDir);
            static LMatrix MMatrixPerspectiveFovLH
            (
                float FovAngleY,
                float AspectRatio,
                float NearZ,
                float FarZ
            );
        };



        
    }
}

