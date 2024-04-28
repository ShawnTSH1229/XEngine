#pragma once
template<typename T>
inline bool CopyAndReturnNotEqual(T& A, T B)
{
	const bool bOut = A != B;
	A = B;
	return bOut;
}