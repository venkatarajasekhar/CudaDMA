/*
 *  Copyright 2013 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#define WARP_SIZE 32
#define WARP_MASK 0x1f
#define CUDADMA_DMA_TID (threadIdx.x-dma_threadIdx_start)

// Enable the restrict keyword to allow additional compiler optimizations
// Note that this can increase register pressure (see appendix B.2.4 of
// CUDA Programming Guide)
#define ENABLE_RESTRICT

#ifdef ENABLE_RESTRICT
#define RESTRICT __restrict__
#else
#define RESTRICT
#endif

// For doing static assertions.  If you get a static assertion that
// means that there is something wrong with the way you instantiated
// a CudaDMA instance.
template<bool COND> struct CudaDMAStaticAssert;
template<> struct CudaDMAStaticAssert<true> { };
#define STATIC_ASSERT(condition) do { CudaDMAStaticAssert<(condition)>(); } while (0)

#define GUARD_ZERO(expr) (((expr) == 0) ? 1 : (expr))
#define GUARD_UNDERFLOW(expr) (((expr) < 0) ? 0 : (expr))
#define GUARD_OVERFLOW(expr,max) ((expr < max) ? expr : (max-1))

// Enumeration types for load and store qualifiers.
// For more information on how these work, see the PTX manual.
enum CudaDMALoadQualifier {
  LOAD_CACHE_ALL, // cache at all levels
  LOAD_CACHE_GLOBAL, // cache only in L2
  LOAD_CACHE_STREAMING, // cache all levels, but mark evict first
  LOAD_CACHE_LAST_USE, // invalidates line after use
  LOAD_CACHE_VOLATILE, // don't cache at any level
};

enum CudaDMAStoreQualifier {
  STORE_WRITE_BACK,
  STORE_CACHE_GLOBAL,
  STORE_CACHE_STREAMING,
  STORE_CACHE_WRITE_THROUGH,
};

__device__ __forceinline__ 
void ptx_cudaDMA_barrier_blocking (const int name, const int num_barriers)
{
  asm volatile("bar.sync %0, %1;" : : "r"(name), "r"(num_barriers) : "memory" );
}

__device__ __forceinline__ 
void ptx_cudaDMA_barrier_nonblocking (const int name, const int num_barriers)
{
  asm volatile("bar.arrive %0, %1;" : : "r"(name), "r"(num_barriers) : "memory" );
}

/*****************************************************/
/*           Load functions                          */
/*****************************************************/
template<typename T, bool GLOBAL_LOAD, int LOAD_QUAL>
__device__ __forceinline__
T ptx_cudaDMA_load(const T *src_ptr)
{
  T result;
  STATIC_ASSERT(GLOBAL_LOAD && !GLOBAL_LOAD);
  return result;
}

// No partial function specialization, so just do them all explicitly
/////////////////////////////
// FLOAT
/////////////////////////////
template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,true,LOAD_CACHE_ALL>(const float *src_ptr)
{
  float result;
#if __CUDA_ARCH__ == 350
  // LDG load
  asm volatile("ld.global.nc.ca.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.ca.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,false,LOAD_CACHE_ALL>(const float *src_ptr)
{
  float result;
  asm volatile("ld.ca.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,true,LOAD_CACHE_GLOBAL>(const float *src_ptr)
{
  float result;
#if __CUDA_ARCH__ == 350
  // LDG load
  asm volatile("ld.global.nc.cg.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.cg.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,false,LOAD_CACHE_GLOBAL>(const float *src_ptr)
{
  float result;
  asm volatile("ld.cg.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,true,LOAD_CACHE_STREAMING>(const float *src_ptr)
{
  float result;
#if __CUDA_ARCH__ == 350
  // LDG load
  asm volatile("ld.global.nc.cs.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.cs.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,false,LOAD_CACHE_STREAMING>(const float *src_ptr)
{
  float result;
  asm volatile("ld.cs.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,true,LOAD_CACHE_LAST_USE>(const float *src_ptr)
{
  float result;
  asm volatile("ld.global.lu.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,false,LOAD_CACHE_LAST_USE>(const float *src_ptr)
{
  float result;
  asm volatile("ld.lu.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,true,LOAD_CACHE_VOLATILE>(const float *src_ptr)
{
  float result;
  asm volatile("ld.global.cv.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float ptx_cudaDMA_load<float,false,LOAD_CACHE_VOLATILE>(const float *src_ptr)
{
  float result;
  asm volatile("ld.cv.f32 %0, [%1];" : "=f"(result) : "l"(src_ptr) : "memory");
  return result;
}

/////////////////////////////
// FLOAT2
/////////////////////////////

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,true,LOAD_CACHE_ALL>(const float2 *src_ptr)
{
  float2 result;
#if __CUDA_ARCH__ == 350
  // LDG load
  asm volatile("ld.global.nc.ca.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.ca.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");   
#endif
  return result;
}

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,false,LOAD_CACHE_ALL>(const float2 *src_ptr)
{
  float2 result;
  asm volatile("ld.ca.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");   
  return result;
}

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,true,LOAD_CACHE_GLOBAL>(const float2 *src_ptr)
{
  float2 result;
#if __CUDA_ARCH__ == 350
  // LDG load
  asm volatile("ld.global.nc.cg.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.cg.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,false,LOAD_CACHE_GLOBAL>(const float2 *src_ptr)
{
  float2 result;
  asm volatile("ld.cg.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,true,LOAD_CACHE_STREAMING>(const float2 *src_ptr)
{
  float2 result;
#if __CUDA_ARCH__ == 350
  // LDG load
  asm volatile("ld.global.nc.cs.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.cs.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,false,LOAD_CACHE_STREAMING>(const float2 *src_ptr)
{
  float2 result;
  asm volatile("ld.cs.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,true,LOAD_CACHE_LAST_USE>(const float2 *src_ptr)
{
  float2 result;
  asm volatile("ld.global.lu.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,false,LOAD_CACHE_LAST_USE>(const float2 *src_ptr)
{
  float2 result;
  asm volatile("ld.lu.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,true,LOAD_CACHE_VOLATILE>(const float2 *src_ptr)
{
  float2 result;
  asm volatile("ld.global.cv.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float2 ptx_cudaDMA_load<float2,false,LOAD_CACHE_VOLATILE>(const float2 *src_ptr)
{
  float2 result;
  asm volatile("ld.cv.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  return result;
}

/////////////////////////////
// FLOAT3
/////////////////////////////

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,true,LOAD_CACHE_ALL>(const float3 *src_ptr)
{
  float3 result;
#if __CUDA_ARCH__ == 350
  // LDG loads
  asm volatile("ld.global.nc.ca.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");   
  asm volatile("ld.global.nc.ca.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.ca.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");   
  asm volatile("ld.global.ca.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,false,LOAD_CACHE_ALL>(const float3 *src_ptr)
{
  float3 result;
  asm volatile("ld.ca.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");   
  asm volatile("ld.ca.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,true,LOAD_CACHE_GLOBAL>(const float3 *src_ptr)
{
  float3 result;
#if __CUDA_ARCH__ == 350
  // LDG loads
  asm volatile("ld.global.nc.cg.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.global.nc.cg.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.cg.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.global.cg.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,false,LOAD_CACHE_GLOBAL>(const float3 *src_ptr)
{
  float3 result;
  asm volatile("ld.cg.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.cg.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,true,LOAD_CACHE_STREAMING>(const float3 *src_ptr)
{
  float3 result;
#if __CUDA_ARCH__ == 350
  // LDG loads
  asm volatile("ld.global.nc.cs.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.global.nc.cs.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.cs.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.global.cs.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,false,LOAD_CACHE_STREAMING>(const float3 *src_ptr)
{
  float3 result;
  asm volatile("ld.cs.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.cs.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,true,LOAD_CACHE_LAST_USE>(const float3 *src_ptr)
{
  float3 result;
  asm volatile("ld.global.lu.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.global.lu.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,false,LOAD_CACHE_LAST_USE>(const float3 *src_ptr)
{
  float3 result;
  asm volatile("ld.lu.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.lu.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,true,LOAD_CACHE_VOLATILE>(const float3 *src_ptr)
{
  float3 result;
  asm volatile("ld.global.cv.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.global.cv.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float3 ptx_cudaDMA_load<float3,false,LOAD_CACHE_VOLATILE>(const float3 *src_ptr)
{
  float3 result;
  asm volatile("ld.cv.v2.f32 {%0,%1}, [%2];" : "=f"(result.x), "=f"(result.y) : "l"(src_ptr) : "memory");
  asm volatile("ld.cv.f32 %0, [%1+8];" : "=f"(result.z) : "l"(src_ptr) : "memory");
  return result;
}

/////////////////////////////
// FLOAT4
/////////////////////////////

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,true,LOAD_CACHE_ALL>(const float4 *src_ptr)
{
  float4 result;
#if __CUDA_ARCH__ == 350
  // LDG load
  asm volatile("ld.global.nc.ca.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.ca.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,false,LOAD_CACHE_ALL>(const float4 *src_ptr)
{
  float4 result;
  asm volatile("ld.ca.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,true,LOAD_CACHE_GLOBAL>(const float4 *src_ptr)
{
  float4 result;
#if __CUDA_ARCH__ == 350
  // LDG load
  asm volatile("ld.global.nc.cg.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.cg.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,false,LOAD_CACHE_GLOBAL>(const float4 *src_ptr)
{
  float4 result;
  asm volatile("ld.cg.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,true,LOAD_CACHE_STREAMING>(const float4 *src_ptr)
{
  float4 result;
#if __CUDA_ARCH__ == 350
  // LDG load
  asm volatile("ld.global.nc.cs.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
#else
  asm volatile("ld.global.cs.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
#endif
  return result;
}

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,false,LOAD_CACHE_STREAMING>(const float4 *src_ptr)
{
  float4 result;
  asm volatile("ld.cs.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,true,LOAD_CACHE_LAST_USE>(const float4 *src_ptr)
{
  float4 result;
  asm volatile("ld.global.lu.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,false,LOAD_CACHE_LAST_USE>(const float4 *src_ptr)
{
  float4 result;
  asm volatile("ld.lu.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,true,LOAD_CACHE_VOLATILE>(const float4 *src_ptr)
{
  float4 result;
  asm volatile("ld.global.cv.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
  return result;
}

template<>
__device__ __forceinline__
float4 ptx_cudaDMA_load<float4,false,LOAD_CACHE_VOLATILE>(const float4 *src_ptr)
{
  float4 result;
  asm volatile("ld.cv.v4.f32 {%0,%1,%2,%3}, [%4];" : "=f"(result.x), "=f"(result.y), "=f"(result.z), "=f"(result.w) : "l"(src_ptr) : "memory");
  return result;
}

/*****************************************************/
/*           Store functions                         */
/*****************************************************/
template<typename T, int STORE_QUAL>
__device__ __forceinline__
void ptx_cudaDMA_store(const T &src_val, T *dst_ptr)
{
  // This template should never be instantiated
  STATIC_ASSERT(STORE_QUAL < 0);
}

/////////////////////////////
// FLOAT
/////////////////////////////

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float,STORE_WRITE_BACK>(const float &src_val, float *dst_ptr)
{
  asm volatile("st.wb.f32 [%0], %1;" :  : "l"(dst_ptr), "f"(src_val) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float,STORE_CACHE_GLOBAL>(const float &src_val, float *dst_ptr)
{
  asm volatile("st.cg.f32 [%0], %1;" :  : "l"(dst_ptr), "f"(src_val) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float,STORE_CACHE_STREAMING>(const float &src_val, float *dst_ptr)
{
  asm volatile("st.cs.f32 [%0], %1;" :  : "l"(dst_ptr), "f"(src_val) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float,STORE_CACHE_WRITE_THROUGH>(const float &src_val, float *dst_ptr)
{
  asm volatile("st.wt.f32 [%0], %1;" :  : "l"(dst_ptr), "f"(src_val) : "memory");
}

/////////////////////////////
// FLOAT2
/////////////////////////////

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float2,STORE_WRITE_BACK>(const float2 &src_val, float2 *dst_ptr)
{
  asm volatile("st.wb.v2.f32 [%0], {%1,%2};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float2,STORE_CACHE_GLOBAL>(const float2 &src_val, float2 *dst_ptr)
{
  asm volatile("st.cg.v2.f32 [%0], {%1,%2};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float2,STORE_CACHE_STREAMING>(const float2 &src_val, float2 *dst_ptr)
{
  asm volatile("st.cs.v2.f32 [%0], {%1,%2};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float2,STORE_CACHE_WRITE_THROUGH>(const float2 &src_val, float2 *dst_ptr)
{
  asm volatile("st.wt.v2.f32 [%0], {%1,%2};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y) : "memory");
}

/////////////////////////////
// FLOAT3
/////////////////////////////

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float3,STORE_WRITE_BACK>(const float3 &src_val, float3 *dst_ptr)
{
  asm volatile("st.wb.v2.f32 [%0], {%1,%2};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y) : "memory");
  asm volatile("st.wb.f32 [%0+8], %1;" :  : "l"(dst_ptr), "f"(src_val.z) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float3,STORE_CACHE_GLOBAL>(const float3 &src_val, float3 *dst_ptr)
{
  asm volatile("st.cg.v2.f32 [%0], {%1,%2};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y) : "memory");
  asm volatile("st.cg.f32 [%0+8], %1;" :  : "l"(dst_ptr), "f"(src_val.z) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float3,STORE_CACHE_STREAMING>(const float3 &src_val, float3 *dst_ptr)
{
  asm volatile("st.cs.v2.f32 [%0], {%1,%2};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y) : "memory");
  asm volatile("st.cs.f32 [%0+8], %1;" :  : "l"(dst_ptr), "f"(src_val.z) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float3,STORE_CACHE_WRITE_THROUGH>(const float3 &src_val, float3 *dst_ptr)
{
  asm volatile("st.wt.v2.f32 [%0], {%1,%2};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y) : "memory");
  asm volatile("st.wt.f32 [%0+8], %1;" :  : "l"(dst_ptr), "f"(src_val.z) : "memory");
}

/////////////////////////////
// FLOAT4
/////////////////////////////

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float4,STORE_WRITE_BACK>(const float4 &src_val, float4 *dst_ptr)
{
  asm volatile("st.wb.v4.f32 [%0], {%1,%2,%3,%4};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y), "f"(src_val.z), "f"(src_val.w) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float4,STORE_CACHE_GLOBAL>(const float4 &src_val, float4 *dst_ptr)
{
  asm volatile("st.cg.v4.f32 [%0], {%1,%2,%3,%4};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y), "f"(src_val.z), "f"(src_val.w) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float4,STORE_CACHE_STREAMING>(const float4 &src_val, float4 *dst_ptr)
{
  asm volatile("st.cs.v4.f32 [%0], {%1,%2,%3,%4};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y), "f"(src_val.z), "f"(src_val.w) : "memory");
}

template<>
__device__ __forceinline__
void ptx_cudaDMA_store<float4,STORE_CACHE_WRITE_THROUGH>(const float4 &src_val, float4 *dst_ptr)
{
  asm volatile("st.wt.v4.f32 [%0], {%1,%2,%3,%4};" :  : "l"(dst_ptr), "f"(src_val.x), "f"(src_val.y), "f"(src_val.z), "f"(src_val.w) : "memory");
}

// Have a special namespace for our meta-programming objects
// so that we can guarantee that they don't interfere with any
// user level code.  
//
// Why metaprogramming?  I just don't trust the compiler.
namespace CudaDMAMeta {
  // A buffer for guaranteeing static access when loading and storing
  // ET = element type
  // NUM_ELMTS = number of elements in the static buffer
  template<typename ET, unsigned NUM_ELMTS>
  class DMABuffer {
  public:
    template<unsigned IDX>
    __device__ __forceinline__
    ET& get_ref(void) { STATIC_ASSERT(IDX < NUM_ELMTS); return buffer[IDX]; }
    template<unsigned IDX>
    __device__ __forceinline__
    const ET& get_ref(void) const { STATIC_ASSERT(IDX < NUM_ELMTS); return buffer[IDX]; }
    template<unsigned IDX>
    __device__ __forceinline__
    ET* get_ptr(void) { STATIC_ASSERT(IDX < NUM_ELMTS); return &buffer[IDX]; }
    template<unsigned IDX>
    __device__ __forceinline__
    const ET* get_ptr(void) const { STATIC_ASSERT(IDX < NUM_ELMTS); return &buffer[IDX]; }
  public:
    template<unsigned IDX, bool GLOBAL_LOAD, int LOAD_QUAL>
    __device__ __forceinline__
    void perform_load(const void *src_ptr)
    {
      perform_load_impl<GUARD_OVERFLOW(IDX,NUM_ELMTS),GLOBAL_LOAD,LOAD_QUAL>(src_ptr);
    }
    template<unsigned IDX, int STORE_QUAL>
    __device__ __forceinline__
    void perform_store(void *dst_ptr) const
    {
      perform_store_impl<GUARD_OVERFLOW(IDX,NUM_ELMTS),STORE_QUAL>(dst_ptr);
    }
  private:
    template<unsigned IDX, bool GLOBAL_LOAD, int LOAD_QUAL>
    __device__ __forceinline__
    void perform_load_impl(const void *src_ptr)
    {
      STATIC_ASSERT(IDX < NUM_ELMTS);
      buffer[IDX] = ptx_cudaDMA_load<ET, GLOBAL_LOAD, LOAD_QUAL>((const ET*)src_ptr);
    }
    template<unsigned IDX, int STORE_QUAL>
    __device__ __forceinline__
    void perform_store_impl(void *dst_ptr) const
    {
      STATIC_ASSERT(IDX < NUM_ELMTS);
      ptx_cudaDMA_store<ET, STORE_QUAL>(buffer[IDX], (ET*)dst_ptr);
    }
  private:
    ET buffer[NUM_ELMTS];
  };

  /********************************************/
  // BufferLoader
  // A class for loading to DMABuffers
  /********************************************/
  template<typename BUFFER, unsigned OFFSET, unsigned STRIDE, unsigned MAX, unsigned IDX, bool GLOBAL_LOAD, int LOAD_QUAL>
  struct BufferLoader {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, unsigned stride)
    {
      buffer.template perform_load<OFFSET+MAX-IDX,GLOBAL_LOAD,LOAD_QUAL>(src);
      BufferLoader<BUFFER,OFFSET,STRIDE,MAX,IDX-STRIDE,GLOBAL_LOAD,LOAD_QUAL>::load_all(buffer, src+stride, stride);
    }
  };

  template<typename BUFFER, unsigned OFFSET, unsigned STRIDE, bool GLOBAL_LOAD, int LOAD_QUAL>
  struct BufferLoader<BUFFER,OFFSET,STRIDE,0,0,GLOBAL_LOAD,LOAD_QUAL>
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, unsigned stride)
    {
      // do nothing
    }
  };

  template<typename BUFFER, unsigned OFFSET, unsigned STRIDE, unsigned MAX, bool GLOBAL_LOAD, int LOAD_QUAL>
  struct BufferLoader<BUFFER,OFFSET,STRIDE,MAX,1,GLOBAL_LOAD,LOAD_QUAL>
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, unsigned stride)
    {
      buffer.template perform_load<OFFSET+MAX-1,GLOBAL_LOAD,LOAD_QUAL>(src);
    }
  };

  /*********************************************/
  // BufferStorer
  // A class for storing from DMABuffers
  /*********************************************/
  template<typename BUFFER, unsigned OFFSET, unsigned STRIDE, unsigned MAX, unsigned IDX, int STORE_QUAL>
  struct BufferStorer {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, unsigned stride)
    {
      buffer.template perform_store<OFFSET+MAX-IDX,STORE_QUAL>(dst);
      BufferStorer<BUFFER,OFFSET,STRIDE,MAX,IDX-STRIDE,STORE_QUAL>::store_all(buffer, dst+stride, stride);
    }
  };

  template<typename BUFFER, unsigned OFFSET, unsigned STRIDE, int STORE_QUAL>
  struct BufferStorer<BUFFER,OFFSET,STRIDE,0,0,STORE_QUAL>
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, unsigned stride)
    {
      // do nothing
    }
  };

  template<typename BUFFER, unsigned OFFSET, unsigned STRIDE, unsigned MAX, int STORE_QUAL>
  struct BufferStorer<BUFFER,OFFSET,STRIDE,MAX,1,STORE_QUAL>
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, unsigned stride)
    {
      buffer.template perform_store<OFFSET+MAX-1,STORE_QUAL>(dst);
    }
  };

  /********************************************/
  // ConditionalBufferLoader
  // A class for loading to DMABuffers with a
  // static upper bound on number of iterations,
  // but a dynamically determined actual iteration count
  /********************************************/
  template<typename BUFFER, int OFFSET, int STRIDE, int MAX, int IDX, bool GLOBAL_LOAD, int LOAD_QUAL>
  struct ConditionalBufferLoader {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, int stride, int actual_max)
    {
      if ((MAX-IDX) < actual_max)
      {
        buffer.template perform_load<OFFSET+MAX-IDX,GLOBAL_LOAD,LOAD_QUAL>(src);
        ConditionalBufferLoader<BUFFER,OFFSET,STRIDE,MAX,IDX-STRIDE,GLOBAL_LOAD,LOAD_QUAL>::load_all(buffer, src+stride, stride, actual_max);
      }
    }
  };

  template<typename BUFFER, int OFFSET, int STRIDE, bool GLOBAL_LOAD, int LOAD_QUAL>
  struct ConditionalBufferLoader<BUFFER,OFFSET,STRIDE,0,0,GLOBAL_LOAD,LOAD_QUAL>
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, int stride, int actual_max)
    {
      // do nothing
    }
  };

  template<typename BUFFER, int OFFSET, int STRIDE, int MAX,bool GLOBAL_LOAD, int LOAD_QUAL>
  struct ConditionalBufferLoader<BUFFER,OFFSET,STRIDE,MAX,1,GLOBAL_LOAD,LOAD_QUAL>
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, int stride, int actual_max)
    {
      if ((MAX-1) < actual_max)
        buffer.template perform_load<OFFSET+MAX-1,GLOBAL_LOAD,LOAD_QUAL>(src);
    }
  };

  /********************************************/
  // ConditionalBufferStorer
  // A class for storing from DMABuffers with a
  // static upper bound on number of iterations,
  // but a dynamically determined actual iteration count
  /********************************************/
  template<typename BUFFER, int OFFSET, int STRIDE, int MAX, int IDX, int STORE_QUAL>
  struct ConditionalBufferStorer {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, int stride, int actual_max)
    {
      if ((MAX-IDX) < actual_max)
      {
        buffer.template perform_store<OFFSET+MAX-IDX,STORE_QUAL>(dst);
        ConditionalBufferStorer<BUFFER,OFFSET,STRIDE,MAX,IDX-STRIDE,STORE_QUAL>::store_all(buffer, dst+stride, stride, actual_max);
      }
    }
  };

  template<typename BUFFER, int OFFSET, int STRIDE, int STORE_QUAL>
  struct ConditionalBufferStorer<BUFFER,OFFSET,STRIDE,0,0,STORE_QUAL>
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, int stride, int actual_max)
    {
      // do nothing
    }
  };

  template<typename BUFFER, int OFFSET, int STRIDE, int MAX, int STORE_QUAL>
  struct ConditionalBufferStorer<BUFFER,OFFSET,STRIDE,MAX,1,STORE_QUAL>
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, int stride, int actual_max)
    {
      if ((MAX-1) < actual_max)
        buffer.template perform_store<OFFSET+MAX-1,STORE_QUAL>(dst);
    }
  };

  /********************************************/
  // NestedBufferLoader 
  // For statically unrolling nested loading loops
  // over DMABuffers
  /********************************************/
  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int OUTER_MAX, int OUTER_IDX, bool GLOBAL_LOAD, int LOAD_QUAL>
  struct NestedBufferLoader 
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, int out_stride, int in_stride)
    {
      BufferLoader<BUFFER,(OUTER_MAX-OUTER_IDX)*OUTER_SCALE,
        INNER_STRIDE,INNER_MAX,INNER_MAX,GLOBAL_LOAD,LOAD_QUAL>::load_all
          (buffer, src, in_stride);
      NestedBufferLoader<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,
        OUTER_MAX,OUTER_IDX-OUTER_STRIDE,GLOBAL_LOAD,LOAD_QUAL>::load_all
          (buffer,src+out_stride,out_stride,in_stride);
    }
  };

  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           bool GLOBAL_LOAD, int LOAD_QUAL>
  struct NestedBufferLoader<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,0,0,GLOBAL_LOAD,LOAD_QUAL>
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, unsigned out_stride, unsigned in_stride)
    {
      // Do nothing
    }
  };

  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int OUTER_MAX, bool GLOBAL_LOAD, int LOAD_QUAL>
  struct NestedBufferLoader<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,OUTER_MAX,1,GLOBAL_LOAD,LOAD_QUAL>
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, unsigned out_stride, unsigned in_stride)
    {
      BufferLoader<BUFFER,(OUTER_MAX-1)*OUTER_SCALE,INNER_STRIDE,INNER_MAX,INNER_MAX,GLOBAL_LOAD,LOAD_QUAL>::load_all
        (buffer, src, in_stride);
    }
  };

  /********************************************/
  // NestedBufferStorer
  // For statically unrolling nested storing loops
  // over DMABuffers
  /********************************************/
  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int OUTER_MAX, int OUTER_IDX, int STORE_QUAL>
  struct NestedBufferStorer
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, int out_stride, int in_stride)
    {
      BufferStorer<BUFFER,(OUTER_MAX-OUTER_IDX)*OUTER_SCALE,
        INNER_STRIDE,INNER_MAX,INNER_MAX,STORE_QUAL>::store_all
          (buffer, dst, in_stride);
      NestedBufferStorer<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,
        OUTER_MAX,OUTER_IDX-OUTER_STRIDE,STORE_QUAL>::store_all
          (buffer,dst+out_stride,out_stride,in_stride);
    }
  };

  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int STORE_QUAL>
  struct NestedBufferStorer<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,0,0,STORE_QUAL>
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, unsigned out_stride, unsigned in_stride)
    {
      // Do nothing
    }
  };

  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int OUTER_MAX, int STORE_QUAL>
  struct NestedBufferStorer<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,OUTER_MAX,1,STORE_QUAL>
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, unsigned out_stride, unsigned in_stride)
    {
      BufferStorer<BUFFER,(OUTER_MAX-1)*OUTER_SCALE,INNER_STRIDE,INNER_MAX,INNER_MAX,STORE_QUAL>::store_all
        (buffer, dst, in_stride);
    }
  };

  /********************************************/
  // NestedConditionalLoader 
  // For statically unrolling nested loading loops
  // over DMABuffers conditionally
  /********************************************/
  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int OUTER_MAX, int OUTER_IDX, bool GLOBAL_LOAD, int LOAD_QUAL>
  struct NestedConditionalLoader
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, int out_stride, int in_stride, int actual_max)
    {
      if ((OUTER_MAX-OUTER_IDX) < actual_max)
      {
        BufferLoader<BUFFER,(OUTER_MAX-OUTER_IDX)*OUTER_SCALE,INNER_STRIDE,
          INNER_MAX,INNER_MAX,GLOBAL_LOAD,LOAD_QUAL>::load_all
            (buffer, src, in_stride);
        NestedConditionalLoader<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,OUTER_MAX,
          OUTER_IDX-OUTER_STRIDE,GLOBAL_LOAD,LOAD_QUAL>::load_all
            (buffer,src+out_stride,out_stride,in_stride,actual_max);
      }
    }
  };
           
  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           bool GLOBAL_LOAD, int LOAD_QUAL>
  struct NestedConditionalLoader<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,0,0,GLOBAL_LOAD,LOAD_QUAL>
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, int out_stride, int in_stride, int actual_max)
    {
      // do nothing
    }
  };

  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int OUTER_MAX, bool GLOBAL_LOAD, int LOAD_QUAL>
  struct NestedConditionalLoader<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,OUTER_MAX,1,GLOBAL_LOAD,LOAD_QUAL>
  {
    static __device__ __forceinline__
    void load_all(BUFFER &buffer, const char *src, int out_stride, int in_stride, int actual_max)
    {
      if ((OUTER_MAX-1) < actual_max)
      {
        BufferLoader<BUFFER,(OUTER_MAX-1)*OUTER_SCALE,INNER_STRIDE,INNER_MAX,
          INNER_MAX,GLOBAL_LOAD,LOAD_QUAL>::load_all
            (buffer, src, in_stride);
      }
    }
  };

  /********************************************/
  // NestedConditionalStorer
  // For statically unrolling nested storing loops
  // over DMABuffers conditionally
  /********************************************/
  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int OUTER_MAX, int OUTER_IDX, int STORE_QUAL>
  struct NestedConditionalStorer
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, int out_stride, int in_stride, int actual_max)
    {
      if ((OUTER_MAX-OUTER_IDX) < actual_max)
      {
        BufferStorer<BUFFER,(OUTER_MAX-OUTER_IDX)*OUTER_SCALE,INNER_STRIDE,
          INNER_MAX,INNER_MAX,STORE_QUAL>::store_all
            (buffer, dst, in_stride);
        NestedConditionalStorer<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,OUTER_MAX,
          OUTER_IDX-OUTER_STRIDE,STORE_QUAL>::store_all
            (buffer,dst+out_stride,out_stride,in_stride,actual_max);
      }
    }
  };
           
  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int STORE_QUAL>
  struct NestedConditionalStorer<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,0,0,STORE_QUAL>
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, int out_stride, int in_stride, int actual_max)
    {
      // do nothing
    }
  };

  template<typename BUFFER, int INNER_STRIDE, int INNER_MAX, int OUTER_SCALE, int OUTER_STRIDE,
           int OUTER_MAX, int STORE_QUAL>
  struct NestedConditionalStorer<BUFFER,INNER_STRIDE,INNER_MAX,OUTER_SCALE,OUTER_STRIDE,OUTER_MAX,1,STORE_QUAL>
  {
    static __device__ __forceinline__
    void store_all(const BUFFER &buffer, char *dst, int out_stride, int in_stride, int actual_max)
    {
      if ((OUTER_MAX-1) < actual_max)
      {
        BufferStorer<BUFFER,(OUTER_MAX-1)*OUTER_SCALE,INNER_STRIDE,INNER_MAX,
          INNER_MAX,STORE_QUAL>::store_all
            (buffer, dst, in_stride);
      }
    }
  };
};

/**
 * This is the base class for CudaDMA and contains most of the baseline
 * functionality that is used for synchronizing all of the CudaDMA instances.
 */
class CudaDMA {
public:
  __device__ CudaDMA(const int dmaID,
                     const int num_dma_threads,
                     const int num_compute_threads,
                     const int dma_threadIdx_start)
    : m_is_dma_thread((int(threadIdx.x)>=dma_threadIdx_start) && (int(threadIdx.x)<(dma_threadIdx_start+num_dma_threads))),
      m_barrierID_empty((dmaID<<1)+1),
      m_barrierID_full(dmaID<<1),
      m_barrier_size(num_dma_threads+num_compute_threads)
  {
  }
public:
  __device__ __forceinline__ void start_async_dma(void) const
  {
    ptx_cudaDMA_barrier_nonblocking(m_barrierID_empty,m_barrier_size);
  }
  __device__ __forceinline__ void wait_for_dma_finish(void) const
  {
    ptx_cudaDMA_barrier_blocking(m_barrierID_full,m_barrier_size);
  }
  __device__ __forceinline__
  bool owns_this_thread(void) const { return m_is_dma_thread; }
protected:
  __device__ __forceinline__ void wait_for_dma_start(void) const
  {
    ptx_cudaDMA_barrier_blocking(m_barrierID_empty,m_barrier_size); 
  }
  __device__ __forceinline__ void finish_async_dma(void) const
  {
    ptx_cudaDMA_barrier_nonblocking(m_barrierID_full,m_barrier_size);
  }
protected:
  const bool m_is_dma_thread;
  const int m_barrierID_empty;
  const int m_barrierID_full;
  const int m_barrier_size;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// CudaDMASequential
//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * CudaDMASequential will transfer data in a contiguous block from one location to another.
 * DO_SYNC - is warp-specialized or not
 * ALIGNMENT - guaranteed alignment of all pointers passed to the instance
 * BYTES_PER_THREAD - maximum number of bytes that can be used for buffering inside the instance
 * BYTES_PER_ELMT - the size of the element to be transfered
 * DMA_THREADS - the number of DMA Threads that are going to be statically allocated
 */
template<bool DO_SYNC, int ALIGNMENT, int BYTES_PER_THREAD=4*ALIGNMENT, int BYTES_PER_ELMT=0, int DMA_THREADS=0>
class CudaDMASequential : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_compute_threads,
                               const int dma_threadIdx_start)
    : CudaDMA(dmaID, DMA_THREADS, num_compute_threads, dma_threadIdx_start)
  {
    // This template should never be instantiated
    STATIC_ASSERT(DO_SYNC && !DO_SYNC);
  }
};

// Number of bytes handled by all DMA threads performing on full load
#define FULL_LD_STRIDE (DMA_THREADS*ALIGNMENT)
// Maximum number of loads that can be performed by a thread based on register constraints
#define BULK_LDS (BYTES_PER_THREAD/ALIGNMENT)
// Number of bytes handled by all DMA threads performing as many loads as possible
// based on the restriction of the number of bytes alloted to them
#define BULK_STEP_STRIDE (FULL_LD_STRIDE*BULK_LDS)
// Number of bulk steps needed
#define BULK_STEPS (BYTES_PER_ELMT/BULK_STEP_STRIDE)
// Number of partial bytes left after performing as many full strides as possible
#define PARTIAL_BYTES (BYTES_PER_ELMT - (BULK_STEPS*BULK_STEP_STRIDE))
// Number of full loads needed in the single partial step
#define PARTIAL_LDS (PARTIAL_BYTES/FULL_LD_STRIDE)
// Number of remaining bytes after performing all the partial loads
#define REMAINING_BYTES (PARTIAL_BYTES - (PARTIAL_LDS*FULL_LD_STRIDE))
// Compute the thread offset
#define THREAD_OFFSET  (CUDADMA_DMA_TID*ALIGNMENT)
// Compute the number of partial bytes that this thread is responsible for
#define THREAD_LEFTOVER (int(REMAINING_BYTES) - int(CUDADMA_DMA_TID*ALIGNMENT))
#define THREAD_PARTIAL_BYTES ((THREAD_LEFTOVER > ALIGNMENT) ? ALIGNMENT : \
                              (THREAD_LEFTOVER < 0) ? 0 : THREAD_LEFTOVER)

#define WARP_SPECIALIZED_UNQUALIFIED_METHODS                                                        \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async(src_ptr);                                                                      \
    wait_xfer_finish(dst_ptr);                                                                      \
  }                                                                                                 \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    SEQUENTIAL_START_XFER_IMPL(false,LOAD_CACHE_ALL,STORE_WRITE_BACK)                               \
  }                                                                                                 \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    CudaDMA::template wait_for_dma_start();                                                         \
    SEQUENTIAL_WAIT_XFER_IMPL(false,LOAD_CACHE_ALL,STORE_WRITE_BACK)                                \
    CudaDMA::template finish_async_dma();                                                           \
  }

#define WARP_SPECIALIZED_QUALIFIED_METHODS                                                          \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async<DMA_GLOBAL_LOAD>(src_ptr);                                                     \
    wait_xfer_finish<DMA_GLOBAL_LOAD>(dst_ptr);                                                     \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    SEQUENTIAL_START_XFER_IMPL(DMA_GLOBAL_LOAD,LOAD_CACHE_ALL,STORE_WRITE_BACK)                     \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    CudaDMA::template wait_for_dma_start();                                                         \
    SEQUENTIAL_WAIT_XFER_IMPL(DMA_GLOBAL_LOAD,LOAD_CACHE_ALL,STORE_WRITE_BACK)                      \
    CudaDMA::template finish_async_dma();                                                           \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL>(src_ptr);                        \
    wait_xfer_finish<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL>(dst_ptr);                        \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    SEQUENTIAL_START_XFER_IMPL(DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL)                        \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    CudaDMA::template wait_for_dma_start();                                                         \
    SEQUENTIAL_WAIT_XFER_IMPL(DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL)                         \
    CudaDMA::template finish_async_dma();                                                           \
  }

#define NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS                                                    \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async(src_ptr);                                                                      \
    wait_xfer_finish(dst_ptr);                                                                      \
  }                                                                                                 \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    SEQUENTIAL_START_XFER_IMPL(false,LOAD_CACHE_ALL,STORE_WRITE_BACK)                               \
  }                                                                                                 \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    SEQUENTIAL_WAIT_XFER_IMPL(false,LOAD_CACHE_ALL,STORE_WRITE_BACK)                                \
  }

#define NON_WARP_SPECIALIZED_QUALIFIED_METHODS                                                      \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async<DMA_GLOBAL_LOAD>(src_ptr);                                                     \
    wait_xfer_finish<DMA_GLOBAL_LOAD>(dst_ptr);                                                     \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    SEQUENTIAL_START_XFER_IMPL(DMA_GLOBAL_LOAD,LOAD_CACHE_ALL,STORE_WRITE_BACK)                     \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    SEQUENTIAL_WAIT_XFER_IMPL(DMA_GLOBAL_LOAD,LOAD_CACHE_ALL,STORE_WRITE_BACK)                      \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL>(src_ptr);                        \
    wait_xfer_finish<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL>(dst_ptr);                        \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    SEQUENTIAL_START_XFER_IMPL(DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL)                        \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    SEQUENTIAL_WAIT_XFER_IMPL(DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL)                         \
  }

#ifdef DEBUG_CUDADMA
#define HANDLE_LOAD_4_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                                       \
  const char *partial_ptr = ((const char*)src_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                    \
  switch (this->dma_partial_bytes)                                                                  \
  {                                                                                                 \
    case 0:                                                                                         \
      break;                                                                                        \
    case 4:                                                                                         \
      partial_buffer = ptx_cudaDMA_load<float,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)partial_ptr);  \
      break;                                                                                        \
    default:                                                                                        \
      assert(false);                                                                                \
  }
#else
#define HANDLE_LOAD_4_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                                       \
  const char *partial_ptr = ((const char*)src_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                    \
  switch (this->dma_partial_bytes)                                                                  \
  {                                                                                                 \
    case 0:                                                                                         \
      break;                                                                                        \
    case 4:                                                                                         \
      partial_buffer = ptx_cudaDMA_load<float,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)partial_ptr);  \
      break;                                                                                        \
  }
#endif

#ifdef DEBUG_CUDADMA
#define HANDLE_STORE_4_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                                \
  char *partial_ptr = ((char*)dst_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                          \
  switch (this->dma_partial_bytes)                                                            \
  {                                                                                           \
    case 0:                                                                                   \
      break;                                                                                  \
    case 4:                                                                                   \
      ptx_cudaDMA_store<float,DMA_STORE_QUAL>(partial_buffer, (float*)partial_ptr);           \
      break;                                                                                  \
    default:                                                                                  \
      assert(false);                                                                          \
  }
#else
#define HANDLE_STORE_4_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                                \
  char *partial_ptr = ((char*)dst_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                          \
  switch (this->dma_partial_bytes)                                                            \
  {                                                                                           \
    case 0:                                                                                   \
      break;                                                                                  \
    case 4:                                                                                   \
      ptx_cudaDMA_store<float,DMA_STORE_QUAL>(partial_buffer, (float*)partial_ptr);           \
      break;                                                                                  \
  }
#endif

#ifdef DEBUG_CUDADMA
#define HANDLE_LOAD_8_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                                           \
  const char *partial_ptr = ((const char*)src_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                        \
  switch (this->dma_partial_bytes)                                                                      \
  {                                                                                                     \
    case 0:                                                                                             \
      break;                                                                                            \
    case 4:                                                                                             \
      partial_buffer.x = ptx_cudaDMA_load<float,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)partial_ptr);    \
      break;                                                                                            \
    case 8:                                                                                             \
      partial_buffer = ptx_cudaDMA_load<float2,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)partial_ptr);    \
      break;                                                                                            \
    default:                                                                                            \
      assert(false);                                                                                    \
  }
#else
#define HANDLE_LOAD_8_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                                           \
  const char *partial_ptr = ((const char*)src_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                        \
  switch (this->dma_partial_bytes)                                                                      \
  {                                                                                                     \
    case 0:                                                                                             \
      break;                                                                                            \
    case 4:                                                                                             \
      partial_buffer.x = ptx_cudaDMA_load<float,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)partial_ptr);    \
      break;                                                                                            \
    case 8:                                                                                             \
      partial_buffer = ptx_cudaDMA_load<float2,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)partial_ptr);    \
      break;                                                                                            \
  }
#endif

#ifdef DEBUG_CUDADMA
#define HANDLE_STORE_8_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                          \
  char *partial_ptr = ((char*)dst_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                    \
  switch (this->dma_partial_bytes)                                                      \
  {                                                                                     \
    case 0:                                                                             \
      break;                                                                            \
    case 4:                                                                             \
      ptx_cudaDMA_store<float,DMA_STORE_QUAL>(partial_buffer.x, (float*)partial_ptr);   \
      break;                                                                            \
    case 8:                                                                             \
      ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(partial_buffer, (float2*)partial_ptr);   \
      break;                                                                            \
    default:                                                                            \
      assert(false);                                                                    \
  }
#else
#define HANDLE_STORE_8_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                          \
  char *partial_ptr = ((char*)dst_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                    \
  switch (this->dma_partial_bytes)                                                      \
  {                                                                                     \
    case 0:                                                                             \
      break;                                                                            \
    case 4:                                                                             \
      ptx_cudaDMA_store<float,DMA_STORE_QUAL>(partial_buffer.x, (float*)partial_ptr);   \
      break;                                                                            \
    case 8:                                                                             \
      ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(partial_buffer, (float2*)partial_ptr);   \
      break;                                                                            \
  }
#endif

#ifdef DEBUG_CUDADMA
#define HANDLE_LOAD_16_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                                          \
  const char *partial_ptr = ((const char*)src_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                        \
  switch (this->dma_partial_bytes)                                                                      \
  {                                                                                                     \
    case 0:                                                                                             \
      break;                                                                                            \
    case 4:                                                                                             \
      partial_buffer.x = ptx_cudaDMA_load<float,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)partial_ptr);    \
      break;                                                                                            \
    case 8:                                                                                             \
      {                                                                                                 \
        float2 temp2 = ptx_cudaDMA_load<float2,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)partial_ptr);    \
        partial_buffer.x = temp2.x;                                                                     \
        partial_buffer.y = temp2.y;                                                                     \
        break;                                                                                          \
      }                                                                                                 \
    case 12:                                                                                            \
      {                                                                                                 \
        float3 temp3 = ptx_cudaDMA_load<float3,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float3*)partial_ptr);    \
        partial_buffer.x = temp3.x;                                                                     \
        partial_buffer.y = temp3.y;                                                                     \
        partial_buffer.z = temp3.z;                                                                     \
        break;                                                                                          \
      }                                                                                                 \
    case 16:                                                                                            \
      partial_buffer = ptx_cudaDMA_load<float4,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float4*)partial_ptr);    \
      break;                                                                                            \
    default:                                                                                            \
      assert(false);                                                                                    \
  }
#else
#define HANDLE_LOAD_16_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                                          \
  const char *partial_ptr = ((const char*)src_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                        \
  switch (this->dma_partial_bytes)                                                                      \
  {                                                                                                     \
    case 0:                                                                                             \
      break;                                                                                            \
    case 4:                                                                                             \
      partial_buffer.x = ptx_cudaDMA_load<float,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)partial_ptr);    \
      break;                                                                                            \
    case 8:                                                                                             \
      {                                                                                                 \
        float2 temp2 = ptx_cudaDMA_load<float2,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)partial_ptr);    \
        partial_buffer.x = temp2.x;                                                                     \
        partial_buffer.y = temp2.y;                                                                     \
        break;                                                                                          \
      }                                                                                                 \
    case 12:                                                                                            \
      {                                                                                                 \
        float3 temp3 = ptx_cudaDMA_load<float3,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float3*)partial_ptr);    \
        partial_buffer.x = temp3.x;                                                                     \
        partial_buffer.y = temp3.y;                                                                     \
        partial_buffer.z = temp3.z;                                                                     \
        break;                                                                                          \
      }                                                                                                 \
    case 16:                                                                                            \
      partial_buffer = ptx_cudaDMA_load<float4,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float4*)partial_ptr);    \
      break;                                                                                            \
  }
#endif

#ifdef DEBUG_CUDADMA
#define HANDLE_STORE_16_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                         \
  char *partial_ptr = ((char*)dst_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                    \
  switch (this->dma_partial_bytes)                                                      \
  {                                                                                     \
    case 0:                                                                             \
      break;                                                                            \
    case 4:                                                                             \
      ptx_cudaDMA_store<float,DMA_STORE_QUAL>(partial_buffer.x, (float*)partial_ptr);   \
      break;                                                                            \
    case 8:                                                                             \
      {                                                                                 \
        float2 temp2;                                                                   \
        temp2.x = partial_buffer.x;                                                     \
        temp2.y = partial_buffer.y;                                                     \
        ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(temp2, (float2*)partial_ptr);          \
        break;                                                                          \
      }                                                                                 \
    case 12:                                                                            \
      {                                                                                 \
        float3 temp3;                                                                   \
        temp3.x = partial_buffer.x;                                                     \
        temp3.y = partial_buffer.y;                                                     \
        temp3.z = partial_buffer.z;                                                     \
        ptx_cudaDMA_store<float3,DMA_STORE_QUAL>(temp3, (float3*)partial_ptr);          \
        break;                                                                          \
      }                                                                                 \
    case 16:                                                                            \
      ptx_cudaDMA_store<float4,DMA_STORE_QUAL>(partial_buffer, (float4*)partial_ptr);   \
      break;                                                                            \
    default:                                                                            \
      assert(false);                                                                    \
  }
#else
#define HANDLE_STORE_16_PARTIAL_BYTES(NUM_PREV_LOADS,LD_STRIDE)                         \
  char *partial_ptr = ((char*)dst_ptr) + (NUM_PREV_LOADS*LD_STRIDE);                    \
  switch (this->dma_partial_bytes)                                                      \
  {                                                                                     \
    case 0:                                                                             \
      break;                                                                            \
    case 4:                                                                             \
      ptx_cudaDMA_store<float,DMA_STORE_QUAL>(partial_buffer.x, (float*)partial_ptr);   \
      break;                                                                            \
    case 8:                                                                             \
      {                                                                                 \
        float2 temp2;                                                                   \
        temp2.x = partial_buffer.x;                                                     \
        temp2.y = partial_buffer.y;                                                     \
        ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(temp2, (float2*)partial_ptr);          \
        break;                                                                          \
      }                                                                                 \
    case 12:                                                                            \
      {                                                                                 \
        float3 temp3;                                                                   \
        temp3.x = partial_buffer.x;                                                     \
        temp3.y = partial_buffer.y;                                                     \
        temp3.z = partial_buffer.z;                                                     \
        ptx_cudaDMA_store<float3,DMA_STORE_QUAL>(temp3, (float3*)partial_ptr);          \
        break;                                                                          \
      }                                                                                 \
    case 16:                                                                            \
      ptx_cudaDMA_store<float4,DMA_STORE_QUAL>(partial_buffer, (float4*)partial_ptr);   \
      break;                                                                            \
  }
#endif


// one template paramemters, warp-specialized
#define SEQUENTIAL_START_XFER_IMPL(GLOBAL_LOAD,LOAD_QUAL,STORE_QUAL)                                    \
    this->dma_src_ptr = ((const char*)src_ptr) + this->dma_offset;                                      \
    if (BULK_STEPS == 0)                                                                                \
    {                                                                                                   \
      issue_loads<BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>((REMAINING_BYTES>0),PARTIAL_LDS,this->dma_src_ptr);   \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
      issue_loads<BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>(dma_src_ptr);                                         \
      this->dma_src_ptr += BULK_STEP_STRIDE;                                                            \
    }

#define SEQUENTIAL_WAIT_XFER_IMPL(GLOBAL_LOAD,LOAD_QUAL,STORE_QUAL)                                     \
    char *dst_off_ptr = ((char*)dst_ptr) + this->dma_offset;                                            \
    if (BULK_STEPS == 0)                                                                                \
    {                                                                                                   \
      issue_stores<BULK_LDS,STORE_QUAL>((REMAINING_BYTES>0),PARTIAL_LDS,dst_off_ptr);                   \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
      issue_stores<BULK_LDS,STORE_QUAL>(dst_off_ptr);                                                   \
      dst_off_ptr += BULK_STEP_STRIDE;                                                                  \
      for (int i = 0; i < (BULK_STEPS-1); i++)                                                          \
      {                                                                                                 \
        issue_loads<BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>(this->dma_src_ptr);                                 \
        this->dma_src_ptr += BULK_STEP_STRIDE;                                                          \
        issue_stores<BULK_LDS,STORE_QUAL>(dst_off_ptr);                                                 \
        dst_off_ptr += BULK_STEP_STRIDE;                                                                \
      }                                                                                                 \
      issue_loads<BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>((REMAINING_BYTES>0),PARTIAL_LDS,this->dma_src_ptr);   \
      issue_stores<BULK_LDS,STORE_QUAL>((REMAINING_BYTES>0),PARTIAL_LDS,dst_off_ptr);                   \
    }

#define LOCAL_TYPENAME float
#define ALIGNMENT 4
template<int BYTES_PER_THREAD>
class CudaDMASequential<true,ALIGNMENT,BYTES_PER_THREAD,0,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_dma_threads,
                               const int num_compute_threads,
                               const int dma_threadIdx_start,
                               const int elmt_size_in_bytes)
    : CudaDMA(dmaID, num_dma_threads, num_compute_threads, dma_threadIdx_start),
      BYTES_PER_ELMT(elmt_size_in_bytes),
      DMA_THREADS(num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_4_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_4_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }

private:
  const int BYTES_PER_ELMT;
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNMENT 8
template<int BYTES_PER_THREAD>
class CudaDMASequential<true,ALIGNMENT,BYTES_PER_THREAD,0,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_dma_threads,
                               const int num_compute_threads,
                               const int dma_threadIdx_start,
                               const int elmt_size_in_bytes)
    : CudaDMA(dmaID, num_dma_threads, num_compute_threads, dma_threadIdx_start),
      BYTES_PER_ELMT(elmt_size_in_bytes),
      DMA_THREADS(num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_8_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_8_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int BYTES_PER_ELMT;
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float2 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNMENT 16 
template<int BYTES_PER_THREAD>
class CudaDMASequential<true,ALIGNMENT,BYTES_PER_THREAD,0,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_dma_threads,
                               const int num_compute_threads,
                               const int dma_threadIdx_start,
                               const int elmt_size_in_bytes)
    : CudaDMA(dmaID, num_dma_threads, num_compute_threads, dma_threadIdx_start),
      BYTES_PER_ELMT(elmt_size_in_bytes),
      DMA_THREADS(num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_16_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_16_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int BYTES_PER_ELMT;
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float4 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

// one template parameters, non-warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNMENT 4
template<int BYTES_PER_THREAD>
class CudaDMASequential<false,ALIGNMENT,BYTES_PER_THREAD,0,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int elmt_size_in_bytes,
                               const int num_dma_threads = 0,
                               const int dma_threadIdx_start = 0)
    : CudaDMA(0, num_dma_threads, num_dma_threads, dma_threadIdx_start),
      BYTES_PER_ELMT(elmt_size_in_bytes),
      DMA_THREADS((num_dma_threads <= 0) ? blockDim.x : num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
  NON_WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_4_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_4_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int BYTES_PER_ELMT;
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNMENT 8
template<int BYTES_PER_THREAD>
class CudaDMASequential<false,ALIGNMENT,BYTES_PER_THREAD,0,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int elmt_size_in_bytes,
                               const int num_dma_threads= 0,
                               const int dma_threadIdx_start = 0)
    : CudaDMA(0, num_dma_threads, num_dma_threads, dma_threadIdx_start),
      BYTES_PER_ELMT(elmt_size_in_bytes),
      DMA_THREADS((num_dma_threads <= 0) ? blockDim.x : num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
  NON_WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_8_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_8_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int BYTES_PER_ELMT;
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float2 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNMENT 16 
template<int BYTES_PER_THREAD>
class CudaDMASequential<false,ALIGNMENT,BYTES_PER_THREAD,0,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int elmt_size_in_bytes,
                               const int num_dma_threads= 0,
                               const int dma_threadIdx_start = 0)
    : CudaDMA(0, num_dma_threads, num_dma_threads, dma_threadIdx_start),
      BYTES_PER_ELMT(elmt_size_in_bytes),
      DMA_THREADS((num_dma_threads <= 0) ? blockDim.x : num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
  NON_WARP_SPECIALIZED_QUALIFIED_METHODS
public:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_16_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_16_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int BYTES_PER_ELMT;
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float4 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#undef SEQUENTIAL_START_XFER_IMPL
#undef SEQUENTIAL_WAIT_XFER_IMPL

// two template parameters, warp-specialized
#define SEQUENTIAL_START_XFER_IMPL(GLOBAL_LOAD,LOAD_QUAL,STORE_QUAL)                                    \
    this->dma_src_ptr = ((const char*)src_ptr) + this->dma_offset;                                      \
    if (BULK_STEPS == 0)                                                                                \
    {                                                                                                   \
      issue_loads<BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>((REMAINING_BYTES>0),PARTIAL_LDS,this->dma_src_ptr);   \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
      issue_loads<BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>(dma_src_ptr);                                         \
      this->dma_src_ptr += BULK_STEP_STRIDE;                                                            \
    }

#define SEQUENTIAL_WAIT_XFER_IMPL(GLOBAL_LOAD,LOAD_QUAL,STORE_QUAL)                                     \
    char *dst_off_ptr = ((char*)dst_ptr) + this->dma_offset;                                            \
    if (BULK_STEPS == 0)                                                                                \
    {                                                                                                   \
      issue_stores<BULK_LDS,STORE_QUAL>((REMAINING_BYTES>0),PARTIAL_LDS,dst_off_ptr);                   \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
      issue_stores<BULK_LDS,STORE_QUAL>(dst_off_ptr);                                                   \
      dst_off_ptr += BULK_STEP_STRIDE;                                                                  \
      for (int i = 0; i < (BULK_STEPS-1); i++)                                                          \
      {                                                                                                 \
        issue_loads<BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>(this->dma_src_ptr);                                 \
        this->dma_src_ptr += BULK_STEP_STRIDE;                                                          \
        issue_stores<BULK_LDS,STORE_QUAL>(dst_off_ptr);                                                 \
        dst_off_ptr += BULK_STEP_STRIDE;                                                                \
      }                                                                                                 \
      issue_loads<BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>((REMAINING_BYTES>0),PARTIAL_LDS,this->dma_src_ptr);   \
      issue_stores<BULK_LDS,STORE_QUAL>((REMAINING_BYTES>0),PARTIAL_LDS,dst_off_ptr);                   \
    }

#define LOCAL_TYPENAME float
#define ALIGNMENT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMASequential<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_dma_threads,
                               const int num_compute_threads,
                               const int dma_threadIdx_start)
    : CudaDMA(dmaID, num_dma_threads, num_compute_threads, dma_threadIdx_start),
      DMA_THREADS(num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_4_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_4_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNMENT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMASequential<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_dma_threads,
                               const int num_compute_threads,
                               const int dma_threadIdx_start)
    : CudaDMA(dmaID, num_dma_threads, num_compute_threads, dma_threadIdx_start),
      DMA_THREADS(num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_8_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_8_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float2 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNMENT 16
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMASequential<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_dma_threads,
                               const int num_compute_threads,
                               const int dma_threadIdx_start)
    : CudaDMA(dmaID, num_dma_threads, num_compute_threads, dma_threadIdx_start),
      DMA_THREADS(num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_16_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_16_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float4 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

// two template parameters, non-warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNMENT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMASequential<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int num_dma_threads = 0,
                               const int dma_threadIdx_start = 0)
    : CudaDMA(0, num_dma_threads, num_dma_threads, dma_threadIdx_start),
      DMA_THREADS((num_dma_threads <= 0) ? blockDim.x : num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
  NON_WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_4_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_4_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNMENT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMASequential<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int num_dma_threads = 0,
                               const int dma_threadIdx_start = 0)
    : CudaDMA(0, num_dma_threads, num_dma_threads, dma_threadIdx_start),
      DMA_THREADS((num_dma_threads <= 0) ? blockDim.x : num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
  NON_WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_8_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_8_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float2 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNMENT 16 
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMASequential<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int num_dma_threads = 0,
                               const int dma_threadIdx_start = 0)
    : CudaDMA(0, num_dma_threads, num_dma_threads, dma_threadIdx_start),
      DMA_THREADS((num_dma_threads <= 0) ? blockDim.x : num_dma_threads),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
  NON_WARP_SPECIALIZED_QUALIFIED_METHODS
public:
  template<int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(bool has_partial, int full_loads, const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::ConditionalBufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_LOAD_16_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
  template<int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
  }
  template<int DMA_MAX_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(bool has_partial, int full_loads, void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::ConditionalBufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_MAX_LOADS),GUARD_UNDERFLOW(DMA_MAX_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE, full_loads);
    if (has_partial)
    {
      HANDLE_STORE_16_PARTIAL_BYTES(full_loads,FULL_LD_STRIDE);
    }
  }
private:
  const int DMA_THREADS;
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float4 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#undef SEQUENTIAL_START_XFER_IMPL
#undef SEQUENTIAL_WAIT_XFER_IMPL

// three template parameters, warp-specialized 
#define SEQUENTIAL_START_XFER_IMPL(GLOBAL_LOAD,LOAD_QUAL,STORE_QUAL)                          \
    this->dma_src_ptr = ((const char*)src_ptr) + this->dma_offset;                            \
    if (BULK_STEPS == 0)                                                                      \
    {                                                                                         \
      issue_loads<(REMAINING_BYTES>0),PARTIAL_LDS,GLOBAL_LOAD,LOAD_QUAL>(this->dma_src_ptr);  \
    }                                                                                         \
    else                                                                                      \
    {                                                                                         \
      issue_loads<false,BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>(dma_src_ptr);                         \
      this->dma_src_ptr += BULK_STEP_STRIDE;                                                  \
    }

#define SEQUENTIAL_WAIT_XFER_IMPL(GLOBAL_LOAD,LOAD_QUAL,STORE_QUAL)                           \
    char *dst_off_ptr = ((char*)dst_ptr) + this->dma_offset;                                  \
    if (BULK_STEPS == 0)                                                                      \
    {                                                                                         \
      issue_stores<(REMAINING_BYTES>0),PARTIAL_LDS,STORE_QUAL>(dst_off_ptr);                  \
    }                                                                                         \
    else                                                                                      \
    {                                                                                         \
      issue_stores<false,BULK_LDS,STORE_QUAL>(dst_off_ptr);                                   \
      dst_off_ptr += BULK_STEP_STRIDE;                                                        \
      for (int i = 0; i < (BULK_STEPS-1); i++)                                                \
      {                                                                                       \
        issue_loads<false,BULK_LDS,GLOBAL_LOAD,LOAD_QUAL>(this->dma_src_ptr);                 \
        this->dma_src_ptr += BULK_STEP_STRIDE;                                                \
        issue_stores<false,BULK_LDS,STORE_QUAL>(dst_off_ptr);                                 \
        dst_off_ptr += BULK_STEP_STRIDE;                                                      \
      }                                                                                       \
      issue_loads<(REMAINING_BYTES>0),PARTIAL_LDS,GLOBAL_LOAD,LOAD_QUAL>(this->dma_src_ptr);  \
      issue_stores<(REMAINING_BYTES>0),PARTIAL_LDS,STORE_QUAL>(dst_off_ptr);                  \
    }

#define LOCAL_TYPENAME  float
#define ALIGNMENT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMASequential<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_compute_threads,
                               const int dma_threadIdx_start)
    : CudaDMA(dmaID, DMA_THREADS, num_compute_threads, dma_threadIdx_start),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  // Helper methods
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);   
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_LOAD_4_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_STORE_4_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
private:
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float partial_buffer;
};
#undef ALIGNMENT
#undef LOCAL_TYPENAME

#define LOCAL_TYPENAME float2
#define ALIGNMENT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMASequential<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_compute_threads,
                               const int dma_threadIdx_start)
    : CudaDMA(dmaID, DMA_THREADS, num_compute_threads, dma_threadIdx_start),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  // Helper methods
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);   
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_LOAD_8_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_STORE_8_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
private:
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float2 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNMENT 16
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMASequential<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dmaID,
                               const int num_compute_threads,
                               const int dma_threadIdx_start)
    : CudaDMA(dmaID, DMA_THREADS, num_compute_threads, dma_threadIdx_start),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  // Helper methods
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_LOAD_16_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_STORE_16_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
private:
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float4 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

// three template parameters, non-warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNMENT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMASequential<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dma_threadIdx_start = 0)
    : CudaDMA(0, DMA_THREADS, DMA_THREADS, dma_threadIdx_start),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
  NON_WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  // Helper methods
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);   
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_LOAD_4_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_STORE_4_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
private:
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNMENT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMASequential<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dma_threadIdx_start = 0)
    : CudaDMA(0, DMA_THREADS, DMA_THREADS, dma_threadIdx_start),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
  NON_WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  // Helper methods
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);   
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_LOAD_8_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_STORE_8_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
private:
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float2 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNMENT 16
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMASequential<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS> : public CudaDMA {
public:
  __device__ CudaDMASequential(const int dma_threadIdx_start = 0)
    : CudaDMA(0, DMA_THREADS, DMA_THREADS, dma_threadIdx_start),
      dma_offset(THREAD_OFFSET),
      dma_partial_bytes(THREAD_PARTIAL_BYTES)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
  NON_WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  // Helper methods
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>
  __device__ __forceinline__ void issue_loads(const void *RESTRICT src_ptr)
  {
    CudaDMAMeta::BufferLoader<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, (const char*)src_ptr, FULL_LD_STRIDE);
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_LOAD_16_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
  template<bool DMA_PARTIAL_BYTES, int DMA_FULL_LOADS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void issue_stores(void *RESTRICT dst_ptr)
  {
    CudaDMAMeta::BufferStorer<BulkBuffer,0,1,GUARD_UNDERFLOW(DMA_FULL_LOADS),GUARD_UNDERFLOW(DMA_FULL_LOADS),DMA_STORE_QUAL>::store_all(bulk_buffer, (char*)dst_ptr, FULL_LD_STRIDE);
    if (DMA_PARTIAL_BYTES)
    {
      HANDLE_STORE_16_PARTIAL_BYTES(DMA_FULL_LOADS,FULL_LD_STRIDE);
    }
  }
private:
  const int dma_offset;
  const int dma_partial_bytes;
  const char *dma_src_ptr;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  float4 partial_buffer;
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#undef SEQUENTIAL_START_XFER_IMPL
#undef SEQUENTIAL_WAIT_XFER_IMPL

#undef FULL_LD_STRIDE
#undef BULK_LDS
#undef BULK_STEP_STRIDE
#undef BULK_STEPS
#undef PARTIAL_BYTES
#undef PARTIAL_LDS
#undef REMAINING_BYTES
#undef THREAD_OFFSET
#undef THREAD_LEFTOVER
#undef THREAD_PARTIAL_BYTES
#undef WARP_SPECIALIZED_UNQUALIFIED_METHODS
#undef WARP_SPECIALIZED_QUALIFIED_METHODS
#undef NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
#undef NON_WARP_SPECIALIZED_QUALIFIED_METHODS
#undef HANDLE_LOAD_4_PARTIAL_BYTES
#undef HANDLE_STORE_4_PARTIAL_BYTES
#undef HANDLE_LOAD_8_PARTIAL_BYTES
#undef HANDLE_STORE_8_PARTIAL_BYTES
#undef HANDLE_LOAD_16_PARTIAL_BYTES
#undef HANDLE_STORE_16_PARTIAL_BYTES

////////////////////////  End of CudaDMASequential //////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// CudaDMAStrided
//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * CudaDMASequential will transfer a 2D block of data from one location to another.  The user must
 * specify the size of the elements to be transfered (X-dimension) as well as the (Y-dimension).
 * DO-SYNC - is warp-specialized or not
 * ALIGNMENT - guaranteed alignment of all pointers passed to the instance
 * BYTES_PER_THREAD - maximum number of bytes that can be used for buffering inside the instance
 * BYTES_PER_ELMT - the size of each element in bytes
 * NUM_ELMTS - the number of elements to be transferred
 * DMA_THREADS - the number of DMA threads that are going to be used
 */

/**
 * A few notes on the implementation of CudaDMAStrided.
 * 
 * This is a simple description of the strategy for CudaDMAStrided.  There
 * are three primary cases in which problems are placed depending on the
 * size of the elements, the number of DMA threads, and the number of 
 * registers available for outstanding loads.  We define a 'step' to
 * be the issuing a group of loads up to the maximum number permitted
 * by the user-specified number of available registers and then 
 * writing them back into memory.
 *
 * The first case that we handle is the 'split' case.  This occurs 
 * whenever elements require 32 or fewer loads to be transferred.  In
 * this case we can split a warp across multiple elements.  Warps 
 * are still split into powers of 2 to avoid warp divergence (see THREADS_PER_ELMT).
 *
 * The second case that we handle is the 'big' elements case.  Big
 * here is a relative term dependent on the number of DMA threads
 * and the number of permitted outstanding loads.  We define a big
 * element to be one that can't be loaded by all the DMA warps
 * in a single step.  In this case we assign all the DMA warps to
 * a single element at a time and have them perform as many steps
 * as necessary to load the element.  All the warps then move onto
 * the next element.
 *
 * The final case ('full') handles all the remaining problems.  We know in this
 * case that we always have enough warps to 'cover' an element: that is
 * we can always assign enough warps to an element to load it in a single
 * step.  We assign the minimum number of warps necessary to cover an
 * element unless this will result in unused warps due to a low element
 * count.  If we find that there would be unused warps due to a low
 * element count, we assign as many warps as possible to an element
 * such that they will all be busy to maximize memory-level parallelism.
 *
 * After assigning warps for the 'full' case, warps figure out how
 * many total loads they need to perform for each element, and then
 * based on the number of registers they have, they can compute
 * how many elements they can handle in a single step.
 *
 * To optimize we differentiate code paths.  In the case where
 * an entire transfer can be performed in a single step, we have
 * a fast path to enable pre-loading of elements into registers.
 * For cases where we know the exact size of transfers, we
 * have optimized paths as well.  For the unoptimized path
 * we still place upper bounds on the number of required registers
 * to enable the compiler to optimize register allocation.
 */
#define MAX_LDS_PER_THREAD (BYTES_PER_THREAD/ALIGNMENT)

#define LDS_PER_ELMT ((BYTES_PER_ELMT+ALIGNMENT-1)/ALIGNMENT)
#define FULL_LDS_PER_ELMT (BYTES_PER_ELMT/ALIGNMENT)

// Figure out if we need to split a warp across multiple elements
// because the elements are very small (i.e. total loads per element <= 32)
#define SPLIT_WARP (LDS_PER_ELMT <= WARP_SIZE)
#define THREADS_PER_ELMT (LDS_PER_ELMT > (WARP_SIZE/2) ? WARP_SIZE : \
			 LDS_PER_ELMT > (WARP_SIZE/4) ? (WARP_SIZE/2) : \
			 LDS_PER_ELMT > (WARP_SIZE/8) ? (WARP_SIZE/4) : \
			 LDS_PER_ELMT > (WARP_SIZE/16) ? (WARP_SIZE/8) : \
			 LDS_PER_ELMT > (WARP_SIZE/32) ? (WARP_SIZE/16) : WARP_SIZE/32)
#define ELMT_PER_STEP_SPLIT ((DMA_THREADS/THREADS_PER_ELMT) * MAX_LDS_PER_THREAD)
#define ROW_ITERS_SPLIT	 (MAX_LDS_PER_THREAD)
#define HAS_PARTIAL_ELMTS_SPLIT ((NUM_ELMTS % ELMT_PER_STEP_SPLIT) != 0)
#define HAS_PARTIAL_BYTES_SPLIT ((BYTES_PER_ELMT % (THREADS_PER_ELMT*ALIGNMENT)) != 0)
#define COL_ITERS_SPLIT  ((BYTES_PER_ELMT == (THREADS_PER_ELMT*ALIGNMENT)) ? 1 : 0)
#define STEP_ITERS_SPLIT (NUM_ELMTS/ELMT_PER_STEP_SPLIT)

#define NUM_WARPS (DMA_THREADS/WARP_SIZE)
// Next we'll handle the case where all the warps performing as many loads as
// possible in a step can't handle an entire element.
#define BIG_ELMTS ((DMA_THREADS*MAX_LDS_PER_THREAD) < LDS_PER_ELMT)
// This is the number of steps where ALL threads can perform the maximum number of loads
#define MAX_ITERS_BIG (LDS_PER_ELMT/(DMA_THREADS*MAX_LDS_PER_THREAD))
// This is actually whether there are leftovers from the max loading phase
#define HAS_PARTIAL_ELMTS_BIG (((LDS_PER_ELMT % (DMA_THREADS*MAX_LDS_PER_THREAD)) / DMA_THREADS) > 0)
// This is the number of loads to be performed for remaining bytes after max loads
#define PART_ITERS_BIG ((LDS_PER_ELMT % (DMA_THREADS*MAX_LDS_PER_THREAD)) / DMA_THREADS)
#define REMAINING_BYTES_BIG (BYTES_PER_ELMT - (MAX_ITERS_BIG*DMA_THREADS*MAX_LDS_PER_THREAD*ALIGNMENT + \
                                                  PART_ITERS_BIG*DMA_THREADS*ALIGNMENT))
// This is actually whether there are leftovers from the partial loading phase
#define HAS_PARTIAL_BYTES_BIG (REMAINING_BYTES_BIG > 0)
#define STEP_ITERS_BIG (NUM_ELMTS)
// Now handle the case where we don't have to split a warp across multiple elements.
// See if we can have a single warp handle an entire element in a single pass.
// We only do this if every warp will be busy, otherwise we'll allocate many warps 
// to an element to maximize MLP.
#define SINGLE_WARP ((LDS_PER_ELMT <= (WARP_SIZE*MAX_LDS_PER_THREAD)) && (NUM_WARPS <= NUM_ELMTS))
// If we can't do single, figure out the minimum number of warps needed to cover
// the element doing as many loads as possible and allocate those warps.  The idea here is to get
// a small number of warps on each element to minimize the wasting of warps.
#define MINIMUM_COVER ((LDS_PER_ELMT+(WARP_SIZE*MAX_LDS_PER_THREAD)-1)/(WARP_SIZE*MAX_LDS_PER_THREAD))
// Otherwise we'll allocate as many warps as possible to a single element to maximize MLP.
// Try to allocate as many warps as possible to an element, each performing one load
// to maximize MLP, if we exceed the maximum, then split the group of warps across
// multiple elements.  Once we've allocated warps to elements, see how many elements we
// can handle based on the number of outstanding loads each thread can have.
// Include the BIG_ELMTS case to avoid divide by zero errors in template instantiation.
#define MAX_WARPS_PER_ELMT ((LDS_PER_ELMT+WARP_SIZE-1)/WARP_SIZE)
#define WARPS_PER_ELMT (SINGLE_WARP ? 1 : \
                        BIG_ELMTS ? NUM_WARPS : \
                        ((NUM_WARPS/MINIMUM_COVER) <= NUM_ELMTS) ? MINIMUM_COVER : \
                        ((MAX_WARPS_PER_ELMT >= NUM_WARPS) ? NUM_WARPS : MAX_WARPS_PER_ELMT))
// Figure out how many loads need to be done per thread per element (round up)
#define LDS_PER_ELMT_PER_THREAD ((LDS_PER_ELMT+(WARPS_PER_ELMT*WARP_SIZE)-1)/(WARPS_PER_ELMT*WARP_SIZE))
// This assumes that the number of warps allocated to the element were enough to
// cover the size of the element. Note we mask out the result if we're in a BIG_ELMTS
// case because it can lead to divide by zero errors in the template instantiation.
#define ELMT_PER_STEP_PER_THREAD (BIG_ELMTS ? 1 : MAX_LDS_PER_THREAD/LDS_PER_ELMT_PER_THREAD)
// Now we can figure out how many elements we can handle per step by multiplying
// the total number of elements to be handled by each thread in a step by
// the total number of groups of warps (also total groups of threads)
#define ELMT_PER_STEP_FULL (ELMT_PER_STEP_PER_THREAD * (NUM_WARPS/WARPS_PER_ELMT))
#define ROW_ITERS_FULL (ELMT_PER_STEP_PER_THREAD)
#define HAS_PARTIAL_ELMTS_FULL ((NUM_ELMTS % ELMT_PER_STEP_FULL) != 0)
#define HAS_PARTIAL_BYTES_FULL ((BYTES_PER_ELMT % (WARPS_PER_ELMT*WARP_SIZE*ALIGNMENT)) != 0)
#define COL_ITERS_FULL (BYTES_PER_ELMT/(WARPS_PER_ELMT*WARP_SIZE*ALIGNMENT))
#define STEP_ITERS_FULL (NUM_ELMTS/ELMT_PER_STEP_FULL)

#define HAS_PARTIAL_BYTES (SPLIT_WARP ? HAS_PARTIAL_BYTES_SPLIT : \
                           BIG_ELMTS ? HAS_PARTIAL_BYTES_BIG : HAS_PARTIAL_BYTES_FULL)
#define HAS_PARTIAL_ELMTS (SPLIT_WARP ? HAS_PARTIAL_ELMTS_SPLIT : \
                           BIG_ELMTS ? HAS_PARTIAL_ELMTS_BIG : HAS_PARTIAL_ELMTS_FULL)

// Finally, let's compute all the initial values based on the things above.
// First we'll do the split versions
#define ELMT_ID_SPLIT (CUDADMA_DMA_TID/THREADS_PER_ELMT)
#define SPLIT_GROUP_TID (threadIdx.x - (dma_threadIdx_start + (ELMT_ID_SPLIT * THREADS_PER_ELMT)))
#define INIT_SRC_OFFSET_SPLIT(_src_stride) (ELMT_ID_SPLIT * _src_stride + SPLIT_GROUP_TID * ALIGNMENT)
#define INIT_DST_OFFSET_SPLIT(_dst_stride) (ELMT_ID_SPLIT * _dst_stride + SPLIT_GROUP_TID * ALIGNMENT)
#define INIT_SRC_STEP_STRIDE_SPLIT(_src_stride) (ELMT_PER_STEP_SPLIT * _src_stride)
#define INIT_DST_STEP_STRIDE_SPLIT(_dst_stride) (ELMT_PER_STEP_SPLIT * _dst_stride)
#define INIT_SRC_ELMT_STRIDE_SPLIT(_src_stride) ((DMA_THREADS/THREADS_PER_ELMT) * _src_stride)
#define INIT_DST_ELMT_STRIDE_SPLIT(_dst_stride) ((DMA_THREADS/THREADS_PER_ELMT) * _dst_stride)
#define INIT_INTRA_ELMT_STRIDE_SPLIT (THREADS_PER_ELMT * ALIGNMENT) // Shouldn't really matter
#define REMAINING_LOADS_SPLIT (FULL_LDS_PER_ELMT % THREADS_PER_ELMT)
// Three cases:
//     1. group id < remaining loads -> partial bytes is ALIGNMENT
//     2. group id > remaining loads -> partial bytes is 0
//     3. group id == remaining loads -> partial bytes is difference between total bytes and full loads * ALIGNMENT
#define INIT_PARTIAL_BYTES_SPLIT ((SPLIT_GROUP_TID > REMAINING_LOADS_SPLIT) ? 0 : \
                                  (SPLIT_GROUP_TID == REMAINING_LOADS_SPLIT) ? (BYTES_PER_ELMT - (FULL_LDS_PER_ELMT * ALIGNMENT)) : \
                                  ALIGNMENT)
#define REMAINING_ELMTS_SPLIT (NUM_ELMTS % ELMT_PER_STEP_SPLIT)
#define FULL_REMAINING_SPLIT (REMAINING_ELMTS_SPLIT / (DMA_THREADS/THREADS_PER_ELMT))
#define LAST_REMAINING_SPLIT (REMAINING_ELMTS_SPLIT % (DMA_THREADS/THREADS_PER_ELMT))
// Two cases:
//     1. element id < last_remaining -> full_remaining+1
//     2. element id >= last_remaining -> full_remaining
#define INIT_PARTIAL_ELMTS_SPLIT (FULL_REMAINING_SPLIT + \
                                  ((LAST_REMAINING_SPLIT==0) ? 0 : \
                                   ((ELMT_ID_SPLIT >= LAST_REMAINING_SPLIT) ? 0 : 1)))

// Now for the big versions
#define INIT_SRC_OFFSET_BIG (CUDADMA_DMA_TID * ALIGNMENT)
#define INIT_DST_OFFSET_BIG (CUDADMA_DMA_TID * ALIGNMENT)
#define INIT_SRC_STEP_STRIDE_BIG(_src_stride) (_src_stride)
#define INIT_DST_STEP_STRIDE_BIG(_dst_stride) (_dst_stride)
#define INIT_SRC_ELMT_STRIDE_BIG(_src_stride) (_src_stride)
#define INIT_DST_ELMT_STRIDE_BIG(_dst_stride) (_dst_stride)
#define INIT_INTRA_ELMT_STRIDE_BIG (DMA_THREADS * ALIGNMENT)
#define INIT_PARTIAL_ELMTS_BIG (0) // No partial elements in the big elements case
#define INIT_PARTIAL_BYTES_BIG ((CUDADMA_DMA_TID > (REMAINING_BYTES_BIG/ALIGNMENT)) ? 0 : \
                                (CUDADMA_DMA_TID == (REMAINING_BYTES_BIG/ALIGNMENT)) ? \
                                                  (BYTES_PER_ELMT - (FULL_LDS_PER_ELMT * ALIGNMENT)) : ALIGNMENT)

// Now we do the full versions 
#define ELMT_ID_FULL (CUDADMA_DMA_TID/(WARPS_PER_ELMT*WARP_SIZE))
#define FULL_GROUP_TID (threadIdx.x - (dma_threadIdx_start + (ELMT_ID_FULL * WARPS_PER_ELMT * WARP_SIZE)))
#define INIT_SRC_OFFSET_FULL(_src_stride) (ELMT_ID_FULL * _src_stride + FULL_GROUP_TID * ALIGNMENT)
#define INIT_DST_OFFSET_FULL(_dst_stride) (ELMT_ID_FULL * _dst_stride + FULL_GROUP_TID * ALIGNMENT)
#define INIT_SRC_STEP_STRIDE_FULL(_src_stride) (ELMT_PER_STEP_FULL * _src_stride)
#define INIT_DST_STEP_STRIDE_FULL(_dst_stride) (ELMT_PER_STEP_FULL * _dst_stride)
#define INIT_SRC_ELMT_STRIDE_FULL(_src_stride) ((NUM_WARPS/WARPS_PER_ELMT) * _src_stride)
#define INIT_DST_ELMT_STRIDE_FULL(_dst_stride) ((NUM_WARPS/WARPS_PER_ELMT) * _dst_stride)
#define INIT_INTRA_ELMT_STRIDE_FULL (WARPS_PER_ELMT * WARP_SIZE * ALIGNMENT)
#define REMAINING_BYTES_FULL (BYTES_PER_ELMT - (COL_ITERS_FULL*WARPS_PER_ELMT*WARP_SIZE*ALIGNMENT))
#define REMAINING_LOADS_FULL (FULL_LDS_PER_ELMT % (WARPS_PER_ELMT * WARP_SIZE))
// Same three cases as for split
#define INIT_PARTIAL_BYTES_FULL ((REMAINING_BYTES_FULL==0) ? 0 : \
                                 (FULL_GROUP_TID > (REMAINING_BYTES_FULL/ALIGNMENT)) ? 0 : \
                                 (FULL_GROUP_TID == (REMAINING_BYTES_FULL/ALIGNMENT)) ? \
                                                  (BYTES_PER_ELMT - (FULL_LDS_PER_ELMT*ALIGNMENT)) : ALIGNMENT)
#define REMAINING_ELMTS_FULL (NUM_ELMTS % ELMT_PER_STEP_FULL)
#define FULL_REMAINING_FULL (REMAINING_ELMTS_FULL / (DMA_THREADS/(WARPS_PER_ELMT * WARP_SIZE)))
#define LAST_REMAINING_FULL (REMAINING_ELMTS_FULL % (DMA_THREADS/(WARPS_PER_ELMT * WARP_SIZE)))
// Same two cases as for split
#define INIT_PARTIAL_ELMTS_FULL (FULL_REMAINING_FULL + \
                                  ((LAST_REMAINING_FULL==0) ? 0 : \
                                   ((ELMT_ID_FULL < LAST_REMAINING_FULL) ? 1 : 0)))
// We also have one more case here for full warp allocation:
// to determine if our warp is one of the active warps
#define WARP_ID (CUDADMA_DMA_TID/WARP_SIZE)
#define NUM_ACTIVE_WARPS ((NUM_WARPS > (NUM_ELMTS*WARPS_PER_ELMT)) ? NUM_ELMTS*WARPS_PER_ELMT : \
                                                                    (NUM_WARPS - (NUM_WARPS % WARPS_PER_ELMT)))
#define INIT_ACTIVE_WARP (WARP_ID < NUM_ACTIVE_WARPS)
#define ALL_WARPS_ACTIVE (NUM_WARPS == NUM_ACTIVE_WARPS) 

#define INIT_SRC_OFFSET(_src_stride) (SPLIT_WARP ? INIT_SRC_OFFSET_SPLIT(_src_stride) : \
                                      BIG_ELMTS  ? INIT_SRC_OFFSET_BIG : INIT_SRC_OFFSET_FULL(_src_stride))
#define INIT_DST_OFFSET(_dst_stride) (SPLIT_WARP ? INIT_DST_OFFSET_SPLIT(_dst_stride) : \
                                      BIG_ELMTS  ? INIT_DST_OFFSET_BIG : INIT_DST_OFFSET_FULL(_dst_stride))
#define INIT_SRC_STEP_STRIDE(_src_stride) (SPLIT_WARP ? INIT_SRC_STEP_STRIDE_SPLIT(_src_stride) : \
                                           BIG_ELMTS  ? INIT_SRC_STEP_STRIDE_BIG(_src_stride) : INIT_SRC_STEP_STRIDE_FULL(_src_stride))
#define INIT_DST_STEP_STRIDE(_dst_stride) (SPLIT_WARP ? INIT_DST_STEP_STRIDE_SPLIT(_dst_stride) : \
                                           BIG_ELMTS  ? INIT_DST_STEP_STRIDE_BIG(_dst_stride) : INIT_DST_STEP_STRIDE_FULL(_dst_stride))
#define INIT_SRC_ELMT_STRIDE(_src_stride) (SPLIT_WARP ? INIT_SRC_ELMT_STRIDE_SPLIT(_src_stride) : \
                                           BIG_ELMTS  ? INIT_SRC_ELMT_STRIDE_BIG(_src_stride) : INIT_SRC_ELMT_STRIDE_FULL(_src_stride))
#define INIT_DST_ELMT_STRIDE(_dst_stride) (SPLIT_WARP ? INIT_DST_ELMT_STRIDE_SPLIT(_dst_stride) : \
                                           BIG_ELMTS  ? INIT_DST_ELMT_STRIDE_BIG(_dst_stride) : INIT_DST_ELMT_STRIDE_FULL(_dst_stride))
#define INIT_INTRA_ELMT_STRIDE (SPLIT_WARP ? INIT_INTRA_ELMT_STRIDE_SPLIT : \
                                BIG_ELMTS  ? INIT_INTRA_ELMT_STRIDE_BIG : INIT_INTRA_ELMT_STRIDE_FULL)
#define INIT_PARTIAL_BYTES (SPLIT_WARP ? INIT_PARTIAL_BYTES_SPLIT : \
                            BIG_ELMTS  ? INIT_PARTIAL_BYTES_BIG : INIT_PARTIAL_BYTES_FULL)
#define INIT_PARTIAL_ELMTS (SPLIT_WARP ? INIT_PARTIAL_ELMTS_SPLIT : \
                            BIG_ELMTS  ? INIT_PARTIAL_ELMTS_BIG : INIT_PARTIAL_ELMTS_FULL)
#define INIT_PARTIAL_OFFSET (SPLIT_WARP ? 0 : BIG_ELMTS ? 0 : \
                            ((FULL_LDS_PER_ELMT - (FULL_LDS_PER_ELMT % (WARPS_PER_ELMT * WARP_SIZE))) * ALIGNMENT))


template<bool DO_SYNC, int ALIGNMENT, int BYTES_PER_THREAD=4*ALIGNMENT, int BYTES_PER_ELMT=0,
         int DMA_THREADS=0, int NUM_ELMTS=0>
class CudaDMAStrided : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start)
    : CudaDMA(dmaID, DMA_THREADS, num_compute_threads, dma_threadIdx_start)
  {
    // This template should never be instantiated
    STATIC_ASSERT(DO_SYNC && !DO_SYNC);
  }
};

#define WARP_SPECIALIZED_UNQUALIFIED_METHODS                                                        \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async(src_ptr);                                                                      \
    wait_xfer_finish(dst_ptr);                                                                      \
  }                                                                                                 \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    STRIDED_START_XFER_IMPL(false,LOAD_CACHE_ALL,STORE_WRITE_BACK)                                  \
  }                                                                                                 \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    CudaDMA::template wait_for_dma_start();                                                         \
    STRIDED_WAIT_XFER_IMPL(false,LOAD_CACHE_ALL,STORE_WRITE_BACK)                                   \
    CudaDMA::template finish_async_dma();                                                           \
  }

#define WARP_SPECIALIZED_QUALIFIED_METHODS                                                          \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async<DMA_GLOBAL_LOAD>(src_ptr);                                                     \
    wait_xfer_finish<DMA_GLOBAL_LOAD>(dst_ptr);                                                     \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    STRIDED_START_XFER_IMPL(DMA_GLOBAL_LOAD,LOAD_CACHE_ALL,STORE_WRITE_BACK)                        \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    CudaDMA::template wait_for_dma_start();                                                         \
    STRIDED_WAIT_XFER_IMPL(DMA_GLOBAL_LOAD,LOAD_CACHE_ALL,STORE_WRITE_BACK)                         \
    CudaDMA::template finish_async_dma();                                                           \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL>(src_ptr);                        \
    wait_xfer_finish<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL>(dst_ptr);                        \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    STRIDED_START_XFER_IMPL(DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL)                           \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    CudaDMA::template wait_for_dma_start();                                                         \
    STRIDED_WAIT_XFER_IMPL(DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL)                            \
    CudaDMA::template finish_async_dma();                                                           \
  }

#define NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS                                                    \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async(src_ptr);                                                                      \
    wait_xfer_finish(dst_ptr);                                                                      \
  }                                                                                                 \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    STRIDED_START_XFER_IMPL(false,LOAD_CACHE_ALL,STORE_WRITE_BACK)                                  \
  }                                                                                                 \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    STRIDED_WAIT_XFER_IMPL(false,LOAD_CACHE_ALL,STORE_WRITE_BACK)                                   \
  }

#define NON_WARP_SPECIALIZED_QUALIFIED_METHODS                                                      \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async<DMA_GLOBAL_LOAD>(src_ptr);                                                     \
    wait_xfer_finish<DMA_GLOBAL_LOAD>(dst_ptr);                                                     \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    STRIDED_START_XFER_IMPL(DMA_GLOBAL_LOAD,LOAD_CACHE_ALL,STORE_WRITE_BACK)                        \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD>                                                                    \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    STRIDED_WAIT_XFER_IMPL(DMA_GLOBAL_LOAD,LOAD_CACHE_ALL,STORE_WRITE_BACK)                         \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void execute_dma(const void *RESTRICT src_ptr, void *RESTRICT dst_ptr) \
  {                                                                                                 \
    start_xfer_async<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL>(src_ptr);                        \
    wait_xfer_finish<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL>(dst_ptr);                        \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void start_xfer_async(const void *RESTRICT src_ptr)                    \
  {                                                                                                 \
    STRIDED_START_XFER_IMPL(DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL)                           \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>                             \
  __device__ __forceinline__ void wait_xfer_finish(void *RESTRICT dst_ptr)                          \
  {                                                                                                 \
    STRIDED_WAIT_XFER_IMPL(DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL)                            \
  }

#ifdef DEBUG_CUDADMA
#define LOAD_4_PARTIAL_BYTES_IMPL                                                                   \
  template<int DMA_ROW_ITERS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                              \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes)   \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float,                                              \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                 \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes,   \
                                              const int row_iters)                                  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float,                                              \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }
#else
#define LOAD_4_PARTIAL_BYTES_IMPL                                                                   \
  template<int DMA_ROW_ITERS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                              \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes)   \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float,                                              \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                 \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes,   \
                                              const int row_iters)                                  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float,                                              \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }
#endif

#ifdef DEBUG_CUDADMA
#define LOAD_8_PARTIAL_BYTES_IMPL                                                                   \
  template<int DMA_ROW_ITERS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                              \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes)   \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i].x = ptx_cudaDMA_load<float,                                            \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float2,                                             \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)src_ptr);                                     \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                 \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes,   \
                                              const int row_iters)                                  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i].x = ptx_cudaDMA_load<float,                                            \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float2,                                             \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)src_ptr);                                     \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }
#else
#define LOAD_8_PARTIAL_BYTES_IMPL                                                                   \
  template<int DMA_ROW_ITERS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                              \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes)   \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i].x = ptx_cudaDMA_load<float,                                            \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float2,                                             \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)src_ptr);                                     \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                 \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes,   \
                                              const int row_iters)                                  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i].x = ptx_cudaDMA_load<float,                                            \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float2,                                             \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)src_ptr);                                     \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }
#endif

#ifdef DEBUG_CUDADMA
#define LOAD_16_PARTIAL_BYTES_IMPL                                                                  \
  template<int DMA_ROW_ITERS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                              \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes)   \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i].x = ptx_cudaDMA_load<float,                                            \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            float2 temp = ptx_cudaDMA_load<float2,                                                  \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)src_ptr);                                     \
            across_buffer[i].x = temp.x;                                                            \
            across_buffer[i].y = temp.y;                                                            \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 12:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            float3 temp = ptx_cudaDMA_load<float3,                                                  \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float3*)src_ptr);                                     \
            across_buffer[i].x = temp.x;                                                            \
            across_buffer[i].y = temp.y;                                                            \
            across_buffer[i].z = temp.z;                                                            \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 16:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float4,                                             \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float4*)src_ptr);                                     \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                 \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes,   \
                                              const int row_iters)                                  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i].x = ptx_cudaDMA_load<float,                                            \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            float2 temp = ptx_cudaDMA_load<float2,                                                  \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)src_ptr);                                     \
            across_buffer[i].x = temp.x;                                                            \
            across_buffer[i].y = temp.y;                                                            \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 12:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            float3 temp = ptx_cudaDMA_load<float3,                                                  \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float3*)src_ptr);                                     \
            across_buffer[i].x = temp.x;                                                            \
            across_buffer[i].y = temp.y;                                                            \
            across_buffer[i].z = temp.z;                                                            \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 16:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float4,                                             \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float4*)src_ptr);                                     \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }
#else
#define LOAD_16_PARTIAL_BYTES_IMPL                                                                  \
  template<int DMA_ROW_ITERS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                              \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes)   \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i].x = ptx_cudaDMA_load<float,                                            \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            float2 temp = ptx_cudaDMA_load<float2,                                                  \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)src_ptr);                                     \
            across_buffer[i].x = temp.x;                                                            \
            across_buffer[i].y = temp.y;                                                            \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 12:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            float3 temp = ptx_cudaDMA_load<float3,                                                  \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float3*)src_ptr);                                     \
            across_buffer[i].x = temp.x;                                                            \
            across_buffer[i].y = temp.y;                                                            \
            across_buffer[i].z = temp.z;                                                            \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 16:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float4,                                             \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float4*)src_ptr);                                     \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }                                                                                                 \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                 \
  __device__ __forceinline__ void load_across(const char *RESTRICT src_ptr,                         \
                                              const int src_elmt_stride, const int partial_bytes,   \
                                              const int row_iters)                                  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i].x = ptx_cudaDMA_load<float,                                            \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);                                      \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            float2 temp = ptx_cudaDMA_load<float2,                                                  \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float2*)src_ptr);                                     \
            across_buffer[i].x = temp.x;                                                            \
            across_buffer[i].y = temp.y;                                                            \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 12:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            float3 temp = ptx_cudaDMA_load<float3,                                                  \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float3*)src_ptr);                                     \
            across_buffer[i].x = temp.x;                                                            \
            across_buffer[i].y = temp.y;                                                            \
            across_buffer[i].z = temp.z;                                                            \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 16:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            across_buffer[i] = ptx_cudaDMA_load<float4,                                             \
              DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float4*)src_ptr);                                     \
            src_ptr += src_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }
#endif

#ifdef DEBUG_CUDADMA
#define STORE_4_PARTIAL_BYTES_IMPL                                                                  \
  template<int DMA_ROW_ITERS, int DMA_STORE_QUAL>                                                   \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes)  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i], (float*)dst_ptr);             \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }                                                                                                 \
  template<int DMA_STORE_QUAL>                                                                      \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes,  \
                                               const int row_iters)                                 \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i], (float*)dst_ptr);             \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }
#else
#define STORE_4_PARTIAL_BYTES_IMPL                                                                  \
  template<int DMA_ROW_ITERS, int DMA_STORE_QUAL>                                                   \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes)  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i], (float*)dst_ptr);             \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }                                                                                                 \
  template<int DMA_STORE_QUAL>                                                                      \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes,  \
                                               const int row_iters)                                 \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i], (float*)dst_ptr);             \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }
#endif

#ifdef DEBUG_CUDADMA
#define STORE_8_PARTIAL_BYTES_IMPL                                                                  \
  template<int DMA_ROW_ITERS, int DMA_STORE_QUAL>                                                   \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes)  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i].x, (float*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(across_buffer[i], (float2*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }                                                                                                 \
  template<int DMA_STORE_QUAL>                                                                      \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes,  \
                                               const int row_iters)                                 \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i].x, (float2*)dst_ptr);          \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(across_buffer[i], (float2*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }
#else
#define STORE_8_PARTIAL_BYTES_IMPL                                                                  \
  template<int DMA_ROW_ITERS, int DMA_STORE_QUAL>                                                   \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes)  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i].x, (float*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(across_buffer[i], (float2*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }                                                                                                 \
  template<int DMA_STORE_QUAL>                                                                      \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes,  \
                                               const int row_iters)                                 \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i].x, (float*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(across_buffer[i], (float2*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }
#endif

#ifdef DEBUG_CUDADMA
#define STORE_16_PARTIAL_BYTES_IMPL                                                                 \
  template<int DMA_ROW_ITERS, int DMA_STORE_QUAL>                                                   \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes)  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i].x, (float*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            float2 temp;                                                                            \
            temp.x = across_buffer[i].x;                                                            \
            temp.y = across_buffer[i].y;                                                            \
            ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(temp, (float2*)dst_ptr);                       \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 12:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            float3 temp;                                                                            \
            temp.x = across_buffer[i].x;                                                            \
            temp.y = across_buffer[i].y;                                                            \
            temp.z = across_buffer[i].z;                                                            \
            ptx_cudaDMA_store<float3,DMA_STORE_QUAL>(temp, (float3*)dst_ptr);                       \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 16:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float4,DMA_STORE_QUAL>(across_buffer[i], (float4*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }                                                                                                 \
  template<int DMA_STORE_QUAL>                                                                      \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes,  \
                                               const int row_iters)                                 \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i].x, (float*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(across_buffer[i], (float2*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 12:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            float3 temp;                                                                            \
            temp.x = across_buffer[i].x;                                                            \
            temp.y = across_buffer[i].y;                                                            \
            temp.z = across_buffer[i].z;                                                            \
            ptx_cudaDMA_store<float3,DMA_STORE_QUAL>(temp, (float3*)dst_ptr);                       \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 16:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float4,DMA_STORE_QUAL>(across_buffer[i], (float4*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      default:                                                                                      \
        assert(false);                                                                              \
    }                                                                                               \
  }
#else
#define STORE_16_PARTIAL_BYTES_IMPL                                                                 \
  template<int DMA_ROW_ITERS, int DMA_STORE_QUAL>                                                   \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes)  \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i].x, (float*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            float2 temp;                                                                            \
            temp.x = across_buffer[i].x;                                                            \
            temp.y = across_buffer[i].y;                                                            \
            ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(temp, (float2*)dst_ptr);                       \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 12:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            float3 temp;                                                                            \
            temp.x = across_buffer[i].x;                                                            \
            temp.y = across_buffer[i].y;                                                            \
            temp.z = across_buffer[i].z;                                                            \
            ptx_cudaDMA_store<float3,DMA_STORE_QUAL>(temp, (float3*)dst_ptr);                       \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 16:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < DMA_ROW_ITERS; i++)                                                   \
          {                                                                                         \
            ptx_cudaDMA_store<float4,DMA_STORE_QUAL>(across_buffer[i], (float4*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }                                                                                                 \
  template<int DMA_STORE_QUAL>                                                                      \
  __device__ __forceinline__ void store_across(char *RESTRICT dst_ptr,                              \
                                               const int dst_elmt_stride, const int partial_bytes,  \
                                               const int row_iters)                                 \
  {                                                                                                 \
    switch (partial_bytes)                                                                          \
    {                                                                                               \
      case 0:                                                                                       \
        break;                                                                                      \
      case 4:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(across_buffer[i].x, (float*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 8:                                                                                       \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float2,DMA_STORE_QUAL>(across_buffer[i], (float2*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 12:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            float3 temp;                                                                            \
            temp.x = across_buffer[i].x;                                                            \
            temp.y = across_buffer[i].y;                                                            \
            temp.z = across_buffer[i].z;                                                            \
            ptx_cudaDMA_store<float3,DMA_STORE_QUAL>(temp, (float3*)dst_ptr);                       \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
      case 16:                                                                                      \
        {                                                                                           \
          for (int i = 0; i < row_iters; i++)                                                       \
          {                                                                                         \
            ptx_cudaDMA_store<float4,DMA_STORE_QUAL>(across_buffer[i], (float4*)dst_ptr);           \
            dst_ptr += dst_elmt_stride;                                                             \
          }                                                                                         \
          break;                                                                                    \
        }                                                                                           \
    }                                                                                               \
  }
#endif

#if 0
// one template parameter, warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNMENT 4
template<int BYTES_PER_THREAD>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,0,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int elmt_size_in_bytes,
                            const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int elmt_size_in_bytes,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNMENT 8
template<int BYTES_PER_THREAD>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,0,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int elmt_size_in_bytes,
                            const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int elmt_size_in_bytes,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNMENT 16 
template<int BYTES_PER_THREAD>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,0,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int elmt_size_in_bytes,
                            const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int elmt_size_in_bytes,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

// one template, non-warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNMENT 4
template<int BYTES_PER_THREAD>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,0,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int elmt_size_in_bytes,
                            const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int elmt_size_in_bytes,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride,
                            const int num_dma_threads = 0,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNMENT 8
template<int BYTES_PER_THREAD>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,0,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int elmt_size_in_bytes,
                            const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int elmt_size_in_bytes,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride,
                            const int num_dma_threads = 0,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNMENT 16 
template<int BYTES_PER_THREAD>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,0,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int elmt_size_in_bytes,
                            const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int elmt_size_in_bytes,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride,
                            const int num_dma_threads = 0,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

// two template parameters, warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNEMNT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int elmt_stride);
  
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNEMNT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int elmt_stride);
  
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNEMNT 16 
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int elmt_stride);
  
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_dma_threads,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

// two template paramaters, non-warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNEMNT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int num_elements,
                            const int src_stride,
                            const int dst_stride,
                            const int num_dma_threads = 0,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNEMNT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int num_elements,
                            const int src_stride,
                            const int dst_stride,
                            const int num_dma_threads = 0,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNEMNT 16 
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,0,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int num_elements,
                            const int src_stride,
                            const int dst_stride,
                            const int num_dma_threads = 0,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

// three template parameters, warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNEMNT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int elmt_stride);
  
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNEMNT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int elmt_stride);
  
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float16
#define ALIGNEMNT 16
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int elmt_stride);
  
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int num_elements,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

// three template parameters, non-warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNEMNT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int num_elements,
                            const int src_stride,
                            const int dst_stride,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNEMNT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int num_elements,
                            const int src_stride,
                            const int dst_stride,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNEMNT 16 
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int num_elements,
                            const int elmt_stride);

  __device__ CudaDMAStrided(const int num_elements,
                            const int src_stride,
                            const int dst_stride,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT
#endif
// four template parameters, warp-specialized
#define TEMPLATE_FOUR_IMPL                                                                                  \
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, bool DMA_IS_SPLIT, bool DMA_IS_BIG,                     \
           int DMA_STEP_ITERS_SPLIT, int DMA_ROW_ITERS_SPLIT, int DMA_COL_ITERS_SPLIT,                      \
           int DMA_STEP_ITERS_BIG,   int DMA_MAX_ITERS_BIG,   int DMA_PART_ITERS_BIG,                       \
           int DMA_STEP_ITERS_FULL,  int DMA_ROW_ITERS_FULL,  int DMA_COL_ITERS_FULL,                       \
	   bool DMA_PARTIAL_BYTES, bool DMA_PARTIAL_ROWS, bool DMA_ALL_WARPS_ACTIVE>                        \
  __device__ __forceinline__ void execute_start_xfer(const void *RESTRICT src_ptr)                          \
  {                                                                                                         \
    this->dma_src_off_ptr = ((const char*)src_ptr) + this->dma_src_offset;                                  \
    if (DMA_IS_SPLIT)                                                                                       \
    {                                                                                                       \
      if (DMA_STEP_ITERS_SPLIT == 0)                                                                        \
      {                                                                                                     \
	load_all_partial_cases<DMA_PARTIAL_BYTES,DMA_PARTIAL_ROWS,                                          \
          DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    \
      }                                                                                                     \
      else                                                                                                  \
      {                                                                                                     \
	load_all_partial_cases<,DMA_PARTIAL_BYTES,false,                                                    \
          DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    \
      }                                                                                                     \
    }                                                                                                       \
    else if (DMA_IS_BIG)                                                                                    \
    {                                                                                                       \
    }                                                                                                       \
    else                                                                                                    \
    {                                                                                                       \
      if (DMA_ALL_WARPS_ACTIVE)                                                                             \
      {                                                                                                     \
	if (DMA_STEP_ITERS_FULL == 0)                                                                       \
	{                                                                                                   \
	  load_all_partial_cases<DMA_PARTIAL_BYTES,DMA_PARTIAL_ROWS,                                        \
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    \
	}                                                                                                   \
	else                                                                                                \
	{                                                                                                   \
	  load_all_partial_cases<DMA_PARTIAL_BYTES,false,                                                   \
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    \
	}                                                                                                   \
      }                                                                                                     \
      else if (this->dma_active_warp)                                                                       \
      {                                                                                                     \
	if (DMA_STEP_ITERS_FULL == 0)                                                                       \
	{                                                                                                   \
	  load_all_partial_cases<DMA_PARTIAL_BYTES,DMA_PARTIAL_ROWS,                                        \
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    \
	}                                                                                                   \
	else                                                                                                \
	{                                                                                                   \
	  load_all_partial_cases<DMA_PARTIAL_BYTES,false,                                                   \
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    \
	}                                                                                                   \
      }                                                                                                     \
    }                                                                                                       \
  }                                                                                                         \
  template<bool DMA_PARTIAL_BYTES, bool DMA_PARTIAL_ROWS,                                                   \
           int DMA_ROW_ITERS, int DMA_COL_ITERS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                   \
  __device__ __forceinline__ void load_all_partial_cases(const char *RESTRICT src_ptr)                      \
  {                                                                                                         \
    if (!DMA_PARTIAL_BYTES)                                                                                 \
    {                                                                                                       \
      if (!DMA_PARTIAL_ROWS)                                                                                \
      {                                                                                                     \
	load_strided<true/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>          \
			(src_ptr, this->dma_src_elmt_stride,                                                \
			 this->dma_intra_elmt_stride, 0/*no partial bytes*/);                               \
      }                                                                                                     \
      else                                                                                                  \
      {                                                                                                     \
	load_strided_upper<true/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>    \
			(src_ptr, this->dma_src_elmt_stride,                                                \
			 this->dma_intra_elmt_stride, 0/*no partial bytes*/, this->dma_partial_elmts);      \
      }                                                                                                     \
    }                                                                                                       \
    else                                                                                                    \
    {                                                                                                       \
      if (!DMA_PARTIAL_ROWS)                                                                                \
      {                                                                                                     \
        load_strided<false/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>         \
			(src_ptr, this->dma_src_elmt_stride,                                                \
			 this->dma_intra_elmt_stride, this->dma_partial_bytes);                             \
      }                                                                                                     \
      else                                                                                                  \
      {                                                                                                     \
	load_strided_upper<false/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>   \
			(src_ptr, this->dma_src_elmt_stride,                                                \
			 this->dma_intra_elmt_stride, this->dma_partial_bytes, this->dma_partial_elmts);    \
      }                                                                                                     \
    }                                                                                                       \
  }                                                                                                         \
  template<bool DMA_ALL_ACTIVE, int DMA_ROW_ITERS, int DMA_COL_ITERS,                                       \
           bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                         \
  __device__ __forceinline__ void load_strided(const char *RESTRICT src_ptr,                                \
  				  	       const int src_elmt_stride,                                   \
					       const int intra_elmt_stride, const int partial_bytes)        \
  {                                                                                                         \
    CudaDMAMeta::NestedBufferLoader<BulkBuffer,1,DMA_COL_ITERS,DMA_COL_ITERS,                               \
      1,DMA_ROW_ITERS,DMA_ROW_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(                               \
                                        bulk_buffer,src_ptr,src_elmt_stride,intra_elmt_stride);             \
    if (!DMA_ALL_ACTIVE)                                                                                    \
    {                                                                                                       \
      load_across<DMA_ROW_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(src_ptr+this->dma_partial_offset,            \
                         src_elmt_stride, partial_bytes);                                                   \
    }                                                                                                       \
  }                                                                                                         \
  template<bool DMA_ALL_ACTIVE, int DMA_ROW_ITERS_UPPER, int DMA_COL_ITERS,                                 \
           bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                         \
  __device__ __forceinline__ void load_strided_upper(const char *RESTRICT src_ptr,                          \
  						const int src_elmt_stride, const int intra_elmt_stride,     \
						const int partial_bytes, const int row_iters)               \
  {                                                                                                         \
    CudaDMAMeta::NestedConditionalLoader<BulkBuffer,1,DMA_COL_ITERS,DMA_COL_ITERS,                          \
      1,DMA_ROW_ITERS_UPPER,DMA_ROW_ITERS_UPPER,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(                   \
                                  bulk_buffer,src_ptr,src_elmt_stride,intra_elmt_stride,row_iters);         \
    if (!DMA_ALL_ACTIVE)                                                                                    \
    {                                                                                                       \
      load_across<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(src_ptr+this->dma_partial_offset,                          \
			     src_elmt_stride, partial_bytes, row_iters);                                    \
    }                                                                                                       \
  }

#define STRIDED_START_XFER_IMPL(GLOBAL_LOAD,LOAD_QUAL,STORE_QUAL)                                           \
  execute_start_xfer<GLOBAL_LOAD,LOAD_QUAL,SPLIT_WARP,BIG_ELMTS,                                            \
                     STEP_ITERS_SPLIT,ROW_ITERS_SPLIT,COL_ITERS_SPLIT,                                      \
                     STEP_ITERS_BIG,MAX_ITERS_BIG,PART_ITERS_BIG,                                           \
                     STEP_ITERS_FULL,ROW_ITERS_FULL,COL_ITERS_FULL,                                         \
                     HAS_PARTIAL_BYTES,HAS_PARTIAL_ELMTS,ALL_WARPS_ACTIVE>(src_ptr);

#define STRIDED_WAIT_XFER_IMPL(GLOBAL_LOAD,LOAD_QUAL,STORE_QUAL)                                            \
  execute_wait_xfer<GLOBAL_LOAD,LOAD_QUAL,STORE_QUAL,SPLIT_WARP,BIG_ELMTS,                                  \
                    STEP_ITERS_SPLIT,ROW_ITERS_SPLIT,COL_ITERS_SPLIT,                                       \
                    STEP_ITERS_BIG,MAX_ITERS_BIG,PART_ITERS_BIG,                                            \
                    STEP_ITERS_FULL,ROW_ITERS_FULL,COL_ITERS_FULL,                                          \
                    HAS_PARTIAL_BYTES,HAS_PARTIAL_ELMTS,ALL_WARPS_ACTIVE>(dst_ptr);

#define LOCAL_TYPENAME float
#define ALIGNMENT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS, int NUM_ELMTS>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,NUM_ELMTS> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int elmt_stride)
    : CudaDMA(dmaID,DMA_THREADS,num_compute_threads,dma_threadIdx_start),
      dma_src_offset(INIT_SRC_OFFSET(elmt_stride)),
      dma_dst_offset(INIT_DST_OFFSET(elmt_stride)),
      dma_src_step_stride(INIT_SRC_STEP_STRIDE(elmt_stride)),
      dma_dst_step_stride(INIT_DST_STEP_STRIDE(elmt_stride)),
      dma_src_elmt_stride(INIT_SRC_ELMT_STRIDE(elmt_stride)),
      dma_dst_elmt_stride(INIT_DST_ELMT_STRIDE(elmt_stride)),
      dma_intra_elmt_stride(INIT_INTRA_ELMT_STRIDE),
      dma_partial_bytes(INIT_PARTIAL_BYTES),
      dma_partial_offset(INIT_PARTIAL_OFFSET),
      dma_partial_elmts(INIT_PARTIAL_ELMTS),
      dma_active_warp(INIT_ACTIVE_WARP)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
  
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int src_stride,
                            const int dst_stride)
    : CudaDMA(dmaID,DMA_THREADS,num_compute_threads,dma_threadIdx_start),
      dma_src_offset(INIT_SRC_OFFSET(src_stride)),
      dma_dst_offset(INIT_DST_OFFSET(dst_stride)),
      dma_src_step_stride(INIT_SRC_STEP_STRIDE(src_stride)),
      dma_dst_step_stride(INIT_DST_STEP_STRIDE(dst_stride)),
      dma_src_elmt_stride(INIT_SRC_ELMT_STRIDE(src_stride)),
      dma_dst_elmt_stride(INIT_DST_ELMT_STRIDE(dst_stride)),
      dma_intra_elmt_stride(INIT_INTRA_ELMT_STRIDE),
      dma_partial_bytes(INIT_PARTIAL_BYTES),
      dma_partial_offset(INIT_PARTIAL_OFFSET),
      dma_partial_elmts(INIT_PARTIAL_ELMTS),
      dma_active_warp(INIT_ACTIVE_WARP)
  {
    STATIC_ASSERT((BYTES_PER_THREAD/ALIGNMENT) > 0);
    STATIC_ASSERT((BYTES_PER_THREAD%ALIGNMENT) == 0);
  }
public:
  WARP_SPECIALIZED_UNQUALIFIED_METHODS
  WARP_SPECIALIZED_QUALIFIED_METHODS
private:
  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, bool DMA_IS_SPLIT, bool DMA_IS_BIG,                     
           int DMA_STEP_ITERS_SPLIT, int DMA_ROW_ITERS_SPLIT, int DMA_COL_ITERS_SPLIT,                      
           int DMA_STEP_ITERS_BIG,   int DMA_MAX_ITERS_BIG,   int DMA_PART_ITERS_BIG,                       
           int DMA_STEP_ITERS_FULL,  int DMA_ROW_ITERS_FULL,  int DMA_COL_ITERS_FULL,                       
	   bool DMA_PARTIAL_BYTES, bool DMA_PARTIAL_ROWS, bool DMA_ALL_WARPS_ACTIVE>                        
  __device__ __forceinline__ void execute_start_xfer(const void *RESTRICT src_ptr)                          
  {                                                                                                         
    this->dma_src_off_ptr = ((const char*)src_ptr) + this->dma_src_offset;                                  
    if (DMA_IS_SPLIT)                                                                                       
    {                                                                                                       
      if (DMA_STEP_ITERS_SPLIT == 0)                                                                        
      {                                                                                                     
	load_all_partial_cases<DMA_PARTIAL_BYTES,DMA_PARTIAL_ROWS,                                          
          DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    
      }                                                                                                     
      else                                                                                                  
      {                                                                                                     
	load_all_partial_cases<DMA_PARTIAL_BYTES,false,                                                    
          DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    
      }                                                                                                     
    }                                                                                                       
    else if (DMA_IS_BIG)                                                                                    
    {                                                                                                       
    }                                                                                                       
    else                                                                                                    
    {                                                                                                       
      if (DMA_ALL_WARPS_ACTIVE)                                                                             
      {                                                                                                     
	if (DMA_STEP_ITERS_FULL == 0)                                                                       
	{                                                                                                   
	  load_all_partial_cases<DMA_PARTIAL_BYTES,DMA_PARTIAL_ROWS,                                       
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    
	}                                                                                                   
	else                                                                                                
	{                                                                                                   
	  load_all_partial_cases<DMA_PARTIAL_BYTES,false,                                                   
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    
	}                                                                                                   
      }                                                                                                     
      else if (this->dma_active_warp)                                                                       
      {                                                                                                     
	if (DMA_STEP_ITERS_FULL == 0)                                                                       
	{                                                                                                   
	  load_all_partial_cases<DMA_PARTIAL_BYTES,DMA_PARTIAL_ROWS,                                        
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    
	}                                                                                                   
	else                                                                                                
	{                                                                                                   
	  load_all_partial_cases<DMA_PARTIAL_BYTES,false,                                                   
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);    
	}                                                                                                   
      }                                                                                                     
    }                                                                                                       
  }                                                                                                         
  template<bool DMA_PARTIAL_BYTES, bool DMA_PARTIAL_ROWS,                                                   
           int DMA_ROW_ITERS, int DMA_COL_ITERS, bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                   
  __device__ __forceinline__ void load_all_partial_cases(const char *RESTRICT src_ptr)                      
  {                                                                                                         
    if (!DMA_PARTIAL_BYTES)                                                                                 
    {                                                                                                       
      if (!DMA_PARTIAL_ROWS)                                                                                
      {                                                                                                     
	load_strided<true/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>          
			(src_ptr, this->dma_src_elmt_stride,                                                
			 this->dma_intra_elmt_stride, 0/*no partial bytes*/);                               
      }                                                                                                     
      else                                                                                                  
      {                                                                                                     
	load_strided_upper<true/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>    
			(src_ptr, this->dma_src_elmt_stride,                                                
			 this->dma_intra_elmt_stride, 0/*no partial bytes*/, this->dma_partial_elmts);      
      }                                                                                                     
    }                                                                                                       
    else                                                                                                    
    {                                                                                                       
      if (!DMA_PARTIAL_ROWS)                                                                                
      {                                                                                                     
        load_strided<false/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>         
			(src_ptr, this->dma_src_elmt_stride,                                                
			 this->dma_intra_elmt_stride, this->dma_partial_bytes);                             
      }                                                                                                     
      else                                                                                                  
      {                                                                                                     
	load_strided_upper<false/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>   
			(src_ptr, this->dma_src_elmt_stride,                                                
			 this->dma_intra_elmt_stride, this->dma_partial_bytes, this->dma_partial_elmts);    
      }                                                                                                     
    }                                                                                                       
  }                                                                                                         
  template<bool DMA_ALL_ACTIVE, int DMA_ROW_ITERS, int DMA_COL_ITERS,                                       
           bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                         
  __device__ __forceinline__ void load_strided(const char *RESTRICT src_ptr,                                
  				  	       const int src_elmt_stride,                                   
					       const int intra_elmt_stride, const int partial_bytes)        
  {                                                                                                         
    CudaDMAMeta::NestedBufferLoader<BulkBuffer,1,DMA_COL_ITERS,DMA_COL_ITERS,                               
      1,DMA_ROW_ITERS,DMA_ROW_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(                               
                                        bulk_buffer,src_ptr,src_elmt_stride,intra_elmt_stride);             
    if (!DMA_ALL_ACTIVE)                                                                                   
    {                                                                                                       
      load_across<DMA_ROW_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(src_ptr+this->dma_partial_offset,            
                         src_elmt_stride, partial_bytes);                                                   
    }                                                                                                       
  }                                                                                                         
  template<bool DMA_ALL_ACTIVE, int DMA_ROW_ITERS_UPPER, int DMA_COL_ITERS,                                 
           bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL>                                                         
  __device__ __forceinline__ void load_strided_upper(const char *RESTRICT src_ptr,                          
  						const int src_elmt_stride, const int intra_elmt_stride,     
						const int partial_bytes, const int row_iters)               
  {                                                                                                         
    CudaDMAMeta::NestedConditionalLoader<BulkBuffer,1,DMA_COL_ITERS,DMA_COL_ITERS,                          
      1,DMA_ROW_ITERS_UPPER,DMA_ROW_ITERS_UPPER,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(                  
                                  bulk_buffer,src_ptr,src_elmt_stride,intra_elmt_stride,row_iters);        
    if (!DMA_ALL_ACTIVE)                                                                                    
    {                                                                                                       
      load_across<DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(src_ptr+this->dma_partial_offset,                          
			     src_elmt_stride, partial_bytes, row_iters);                                    
    }                                                                                                       
  }

  template<bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL,
           bool DMA_IS_SPLIT, bool DMA_IS_BIG,
           int DMA_STEP_ITERS_SPLIT, int DMA_ROW_ITERS_SPLIT, int DMA_COL_ITERS_SPLIT,
           int DMA_STEP_ITERS_BIG,   int DMA_MAX_ITERS_BIG,   int DMA_PART_ITERS_BIG,
           int DMA_STEP_ITERS_FULL,  int DMA_ROW_ITERS_FULL,  int DMA_COL_ITERS_FULL,
	   bool DMA_PARTIAL_BYTES, bool DMA_PARTIAL_ROWS, bool DMA_ALL_WARPS_ACTIVE>
  __device__ __forceinline__ void execute_wait_xfer(void *RESTRICT dst_ptr) 
  {
    char * dst_off_ptr = ((char*)dst_ptr) + this->dma_dst_offset;
    if (DMA_IS_SPLIT)
    {
      if (DMA_STEP_ITERS_SPLIT == 0)
      {
	store_all_partial_cases<DMA_PARTIAL_BYTES,DMA_PARTIAL_ROWS,
          DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_STORE_QUAL>(dst_off_ptr);
      }
      else
      {
        store_all_partial_cases<DMA_PARTIAL_BYTES,false/*partial rows*/,
          DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_STORE_QUAL>(dst_off_ptr);
	this->dma_src_off_ptr += this->dma_src_step_stride;
	dst_off_ptr += this->dma_dst_step_stride;
	// Only need to do N-1 iterations since we already did the first one
	for (int i = 0; i < (DMA_STEP_ITERS_SPLIT-1); i++)
	{
          load_all_partial_cases<DMA_PARTIAL_BYTES,false/*partial rows*/,
            DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);
          store_all_partial_cases<DMA_PARTIAL_BYTES,false/*partial rows*/,
            DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_STORE_QUAL>(dst_off_ptr);
	  this->dma_src_off_ptr += this->dma_src_step_stride;
	  dst_off_ptr += this->dma_dst_step_stride;
        }
	if (DMA_PARTIAL_ROWS)
	{
          load_all_partial_cases<DMA_PARTIAL_BYTES,true/*partial rows*/,
            DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);
          store_all_partial_cases<DMA_PARTIAL_BYTES,true/*partial rows*/,
            DMA_ROW_ITERS_SPLIT,DMA_COL_ITERS_SPLIT,DMA_STORE_QUAL>(dst_off_ptr);
	}
      }
    }
    else if (DMA_IS_BIG)
    {
      for (int i = 0; i < DMA_STEP_ITERS_BIG; i++)
      {
        perform_copy_elmt<DMA_MAX_ITERS_BIG,DMA_PART_ITERS_BIG,
          DMA_PARTIAL_BYTES,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL,DMA_STORE_QUAL>
            (this->dma_src_off_ptr, dst_off_ptr, this->dma_intra_elmt_stride, this->dma_partial_bytes);
        this->dma_src_off_ptr += this->dma_src_elmt_stride;
        dst_off_ptr += this->dma_dst_elmt_stride;
      }
    }
    else
    {
      if (DMA_ALL_WARPS_ACTIVE)
      {
	if (DMA_STEP_ITERS_FULL == 0)
	{
	  store_all_partial_cases<DMA_PARTIAL_BYTES,DMA_PARTIAL_ROWS,
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_STORE_QUAL>(dst_off_ptr);
	}
	else
	{
	  store_all_partial_cases<DMA_PARTIAL_BYTES,false/*partial rows*/,
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_STORE_QUAL>(dst_off_ptr);
	  this->dma_src_off_ptr += this->dma_src_step_stride;
	  dst_off_ptr += this->dma_dst_step_stride;
	  // Only need to handle N-1 cases now
	  for (int i = 0; i < (DMA_STEP_ITERS_FULL-1); i++)
	  {
            load_all_partial_cases<DMA_PARTIAL_BYTES,false/*partial rows*/,
              DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);
            store_all_partial_cases<DMA_PARTIAL_BYTES,false/*partial rows*/,
              DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_STORE_QUAL>(dst_off_ptr);
	    this->dma_src_off_ptr += this->dma_src_step_stride;
	    dst_off_ptr += this->dma_dst_step_stride;
	  }
	  if (DMA_PARTIAL_ROWS)
	  {
            load_all_partial_cases<DMA_PARTIAL_BYTES,true/*partial rows*/,
              DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);
            store_all_partial_cases<DMA_PARTIAL_BYTES,true/*partial rows*/,
              DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_STORE_QUAL>(dst_off_ptr);
	  }
	}
      }
      else if (this->dma_active_warp)
      {
        if (DMA_STEP_ITERS_FULL == 0)
	{
	  store_all_partial_cases<DMA_PARTIAL_BYTES,DMA_PARTIAL_ROWS,
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_STORE_QUAL>(dst_off_ptr);
	}
	else
	{
	  store_all_partial_cases<DMA_PARTIAL_BYTES,false/*partial rows*/,
            DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_STORE_QUAL>(dst_off_ptr);
	  this->dma_src_off_ptr += this->dma_src_step_stride;
	  dst_off_ptr += this->dma_dst_step_stride;
	  // Only need to handle N-1 cases now
	  for (int i = 0; i < (DMA_STEP_ITERS_FULL-1); i++)
	  {
            load_all_partial_cases<DMA_PARTIAL_BYTES,false/*partial rows*/,
              DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);
            store_all_partial_cases<DMA_PARTIAL_BYTES,false/*partial rows*/,
              DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_STORE_QUAL>(dst_off_ptr);
	    this->dma_src_off_ptr += this->dma_src_step_stride;
	    dst_off_ptr += this->dma_dst_step_stride;
	  }
	  if (DMA_PARTIAL_ROWS)
	  {
            load_all_partial_cases<DMA_PARTIAL_BYTES,true/*partial rows*/,
              DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>(this->dma_src_off_ptr);
            store_all_partial_cases<DMA_PARTIAL_BYTES,true/*partial rows*/,
              DMA_ROW_ITERS_FULL,DMA_COL_ITERS_FULL,DMA_STORE_QUAL>(dst_off_ptr);
	  }
	}
      }
    }
  }

  template<bool DMA_PARTIAL_BYTES, bool DMA_PARTIAL_ROWS,
  	   int DMA_ROW_ITERS, int DMA_COL_ITERS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void store_all_partial_cases(char *RESTRICT dst_ptr)
  {
    if (!DMA_PARTIAL_BYTES)
    {
      if (!DMA_PARTIAL_ROWS)
      {
	store_strided<true/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_STORE_QUAL>
			(dst_ptr, this->dma_dst_elmt_stride,
			 this->dma_intra_elmt_stride, 0/*no partial bytes*/);
      }
      else
      {
	store_strided_upper<true/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_STORE_QUAL>
			(dst_ptr, this->dma_dst_elmt_stride,
			 this->dma_intra_elmt_stride, 0/*no partial bytes*/, this->dma_partial_elmts);
      }
    }
    else
    {
      if (!DMA_PARTIAL_ROWS)
      {
	store_strided<false/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_STORE_QUAL>
			(dst_ptr, this->dma_dst_elmt_stride,
			 this->dma_intra_elmt_stride, this->dma_partial_bytes);
      }
      else
      {
	store_strided_upper<false/*all active*/,DMA_ROW_ITERS,DMA_COL_ITERS,DMA_STORE_QUAL>
			(dst_ptr, this->dma_dst_elmt_stride,
			 this->dma_intra_elmt_stride, this->dma_partial_bytes, this->dma_partial_elmts);
      }
    }
  }

  template<bool DMA_ALL_ACTIVE, int DMA_ROW_ITERS, int DMA_COL_ITERS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void store_strided(char *RESTRICT dst_ptr, const int dst_elmt_stride,
  						const int intra_elmt_stride, const int partial_bytes)
  {
    CudaDMAMeta::NestedBufferStorer<BulkBuffer,1,DMA_COL_ITERS,DMA_COL_ITERS,
      1,DMA_ROW_ITERS,DMA_ROW_ITERS,DMA_STORE_QUAL>::store_all(
          bulk_buffer,dst_ptr,dst_elmt_stride,intra_elmt_stride);
    if (!DMA_ALL_ACTIVE)
    { 
      store_across<DMA_ROW_ITERS,DMA_STORE_QUAL>(dst_ptr+this->dma_partial_offset,
			      dst_elmt_stride,partial_bytes);
    }
  }

  template<bool DMA_ALL_ACTIVE, int DMA_ROW_ITERS_UPPER, int DMA_COL_ITERS, int DMA_STORE_QUAL>
  __device__ __forceinline__ void store_strided_upper(char *RESTRICT dst_ptr, const int dst_elmt_stride,
  						      const int intra_elmt_stride, const int partial_bytes,
						      const int row_iters)
  {
    CudaDMAMeta::NestedConditionalStorer<BulkBuffer,1,DMA_COL_ITERS,DMA_COL_ITERS,
      1,DMA_ROW_ITERS_UPPER,DMA_ROW_ITERS_UPPER,DMA_STORE_QUAL>::store_all(
          bulk_buffer,dst_ptr,dst_elmt_stride,intra_elmt_stride,row_iters);
    if (!DMA_ALL_ACTIVE)
    { 
      store_across<DMA_STORE_QUAL>(dst_ptr+this->dma_partial_offset,
			      dst_elmt_stride,partial_bytes,row_iters);
    } 
  }

  template<int DMA_MAX_ITERS, int DMA_PARTIAL_ITERS, bool DMA_PARTIAL_BYTES,
           bool DMA_GLOBAL_LOAD, int DMA_LOAD_QUAL, int DMA_STORE_QUAL>
  __device__ __forceinline__ void perform_copy_elmt(const char *RESTRICT src_ptr, char *RESTRICT dst_ptr, 
                                                    const int intra_elmt_stride, const int partial_bytes)
  {
    for (int idx = 0; idx < DMA_MAX_ITERS; idx++)
    {
      CudaDMAMeta::BufferLoader<BulkBuffer,0,1,BYTES_PER_THREAD/ALIGNMENT,
        BYTES_PER_THREAD/ALIGNMENT,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, src_ptr, intra_elmt_stride);
      src_ptr += ((BYTES_PER_THREAD/ALIGNMENT)*intra_elmt_stride);
      CudaDMAMeta::BufferStorer<BulkBuffer,0,1,BYTES_PER_THREAD/ALIGNMENT,
        BYTES_PER_THREAD/ALIGNMENT,DMA_STORE_QUAL>::store_all(bulk_buffer, dst_ptr, intra_elmt_stride);
      dst_ptr += ((BYTES_PER_THREAD/ALIGNMENT)*intra_elmt_stride);
    }
    if (DMA_PARTIAL_ITERS > 0)
    {
      CudaDMAMeta::BufferLoader<BulkBuffer,0,1,DMA_PARTIAL_ITERS,
        DMA_PARTIAL_ITERS,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>::load_all(bulk_buffer, src_ptr, intra_elmt_stride);
      src_ptr += (DMA_PARTIAL_ITERS * intra_elmt_stride);
      CudaDMAMeta::BufferStorer<BulkBuffer,0,1,DMA_PARTIAL_ITERS,
        DMA_PARTIAL_ITERS,DMA_STORE_QUAL>::store_all(bulk_buffer, dst_ptr, intra_elmt_stride);
      dst_ptr += (DMA_PARTIAL_ITERS * intra_elmt_stride);
    }
    if (DMA_PARTIAL_BYTES)
    {
      switch (partial_bytes)
      {
        case 0:
          break;
        case 4:
          {
            float tmp = ptx_cudaDMA_load<float,DMA_GLOBAL_LOAD,DMA_LOAD_QUAL>((float*)src_ptr);
            ptx_cudaDMA_store<float,DMA_STORE_QUAL>(tmp, (float*)dst_ptr);
            break;
          }
#ifdef DEBUG_CUDADMA
        default:
          assert(false);
#endif
      }
    }
  }
private:
  LOAD_4_PARTIAL_BYTES_IMPL
  STORE_4_PARTIAL_BYTES_IMPL
private:
  const char *dma_src_off_ptr;
  const unsigned int dma_src_offset;
  const unsigned int dma_dst_offset;
  const unsigned int dma_src_step_stride;
  const unsigned int dma_dst_step_stride;
  const unsigned int dma_src_elmt_stride;
  const unsigned int dma_dst_elmt_stride;
  const unsigned int dma_intra_elmt_stride;
  const unsigned int dma_partial_bytes;
  const unsigned int dma_partial_offset;
  const unsigned int dma_partial_elmts;
  const bool         dma_active_warp;
  typedef CudaDMAMeta::DMABuffer<LOCAL_TYPENAME,BYTES_PER_THREAD/ALIGNMENT> BulkBuffer;
  BulkBuffer bulk_buffer;
  LOCAL_TYPENAME across_buffer[GUARD_ZERO(SPLIT_WARP ? ROW_ITERS_SPLIT : 
  				BIG_ELMTS ? 1 : ROW_ITERS_FULL)*ALIGNMENT/sizeof(LOCAL_TYPENAME)];
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#if 0
#define LOCAL_TYPENAME float2
#define ALIGNEMNT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS, int NUM_ELMTS>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,NUM_ELMTS> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int elmt_stride);
  
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNEMNT 16 
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS, int NUM_ELMTS>
class CudaDMAStrided<true,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,NUM_ELMTS> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int elmt_stride);
  
  __device__ CudaDMAStrided(const int dmaID,
                            const int num_compute_threads,
                            const int dma_threadIdx_start,
                            const int src_stride,
                            const int dst_stride);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

// four template parameters, non-warp-specialized
#define LOCAL_TYPENAME float
#define ALIGNEMNT 4
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS, int NUM_ELMTS>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int elmt_stride);

  __device__ CudaDMAStrided(const int src_stride,
                            const int dst_stride,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float2
#define ALIGNEMNT 8
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS, int NUM_ELMTS>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int elmt_stride);

  __device__ CudaDMAStrided(const int src_stride,
                            const int dst_stride,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT

#define LOCAL_TYPENAME float4
#define ALIGNEMNT 16 
template<int BYTES_PER_THREAD, int BYTES_PER_ELMT, int DMA_THREADS, int NUM_ELMTS>
class CudaDMAStrided<false,ALIGNMENT,BYTES_PER_THREAD,BYTES_PER_ELMT,DMA_THREADS,0> : public CudaDMA {
public:
  __device__ CudaDMAStrided(const int elmt_stride);

  __device__ CudaDMAStrided(const int src_stride,
                            const int dst_stride,
                            const int dma_thread_idx_start = 0);
};
#undef LOCAL_TYPENAME
#undef ALIGNMENT
#endif

#undef WARP_SPECIALIZED_UNQUALIFIED_METHODS
#undef WARP_SPECIALIZED_QUALIFIED_METHODS
#undef NON_WARP_SPECIALIZED_UNQUALIFIED_METHODS
#undef NON_WARP_SPECIALIZED_QUALIFIED_METHODS
#undef LOAD_4_PARTIAL_BYTES_IMPL
#undef LOAD_8_PARTIAL_BYTES_IMPL
#undef LOAD_16_PARTIAL_BYTES_IMPL
#undef STORE_4_PARTIAL_BYTES_IMPL
#undef STORE_8_PARTIAL_BYTES_IMPL
#undef STORE_16_PARTIAL_BYTES_IMPL

#undef MAX_LDS_PER_THREAD
#undef LDS_PER_ELMT
#undef FULL_LDS_PER_ELMT
#undef SPLIT_WARP
#undef THREADS_PER_ELMT
#undef ELMT_PER_STEP_SPLIT
#undef ROW_ITERS_SPLIT
#undef HAS_PARTIAL_ELMTS_SPLIT
#undef HAS_PARTIAL_BYTES_SPLIT
#undef COL_ITERS_SPLIT
#undef STEP_ITERS_SPLIT
#undef NUM_WARPS
#undef BIG_ELMTS
#undef MAX_ITERS_BIG
#undef HAS_PARTIAL_ELMTS_BIG
#undef PART_ITERS_BIG
#undef REMAINING_BYTES_BIG
#undef HAS_PARTIAL_BYTES_BIG
#undef STEP_ITERS_BIG
#undef SINGLE_WARP
#undef MINIMUM_COVER
#undef MAX_WARPS_PER_ELMT
#undef WARPS_PER_ELMT
#undef LDS_PER_ELMT_PER_THREAD
#undef ELMT_PER_STEP_PER_THREAD
#undef ELMT_PER_STEP_FULL
#undef ROW_ITERS_FULL
#undef HAS_PARTIAL_ELMTS_FULL
#undef HAS_PARTIAL_BYTES_FULL
#undef COL_ITERS_FULL
#undef STEP_ITERS_FULL
#undef HAS_PARTIAL_BYTES
#undef HAS_PARTIAL_ELMTS
#undef ELMT_ID_SPLIT
#undef SPLIT_GROUP_TID
#undef INIT_SRC_OFFSET_SPLIT
#undef INIT_DST_OFFSET_SPLIT
#undef INIT_SRC_STEP_STRIDE_SPLIT
#undef INIT_DST_STEP_STRIDE_SPLIT
#undef INIT_SRC_ELMT_STRIDE_SPLIT
#undef INIT_DST_ELMT_STRIDE_SPLIT
#undef INIT_INTRA_ELMT_STRIDE_SPLIT
#undef REMAINING_LOADS_SPLIT
#undef INIT_PARTIAL_BYTES_SPLIT
#undef REMAINING_ELMTS_SPLIT
#undef FULL_REMAINING_SPLIT
#undef LAST_REMAINING_SPLIT
#undef INIT_PARTIAL_ELMTS_SPLIT
#undef INIT_SRC_OFFSET_BIG
#undef INIT_DST_OFFSET_BIG
#undef INIT_SRC_STEP_STRIDE_BIG
#undef INIT_DST_STEP_STRIDE_BIG
#undef INIT_SRC_ELMT_STRIDE_BIG
#undef INIT_DST_ELMT_STRIDE_BIG
#undef INIT_INTRA_ELMT_STRIDE_BIG
#undef INIT_PARTIAL_ELMTS_BIG
#undef INIT_PARTIAL_BYTES_BIG
#undef ELMT_ID_FULL
#undef FULL_GROUP_TID
#undef INIT_SRC_OFFSET_FULL
#undef INIT_DST_OFFSET_FULL
#undef INIT_SRC_STEP_STRIDE_FULL
#undef INIT_DST_STEP_STRIDE_FULL
#undef INIT_SRC_ELMT_STRIDE_FULL
#undef INIT_DST_ELMT_STRIDE_FULL
#undef INIT_INTRA_ELMT_STRIDE_FULL
#undef REMAINING_BYTES_FULL
#undef REMAINING_LOADS_FULL
#undef INIT_PARTIAL_BYTES_FULL
#undef REMAINING_ELMTS_FULL
#undef FULL_REMAINING_FULL
#undef LAST_REMAINING_FULL
#undef INIT_PARTIAL_ELMTS_FULL
#undef WARP_ID
#undef NUM_ACTIVE_WARPS
#undef INIT_ACTIVE_WARP
#undef ALL_WARPS_ACTIVE
#undef INIT_SRC_OFFSET
#undef INIT_DST_OFFSET
#undef INIT_SRC_STEP_STRIDE
#undef INIT_DST_STEP_STRIDE
#undef SRC_STEP_STRIDE
#undef DST_STEP_STRIDE
#undef SRC_ELMT_STRIDE
#undef DST_ELMT_STRIDE
#undef INIT_INTRA_ELMT_STRIDE
#undef INIT_PARTIAL_BYTES
#undef INIT_PARTIAL_ELMTS
#undef INIT_PARTIAL_OFFSET
////////////////////////  End of CudaDMAStrided    //////////////////////////////////////////////////

#undef WARP_SIZE
#undef WARP_MASK
#undef CUDADMA_DMA_TID
#undef RESTRICT
#ifdef ENABLE_RESTRICT
#undef ENABLE_RESTRICT
#endif
#undef STATIC_ASSERT
#undef GUARD_ZERO
#undef GUARD_UNDERFLOW
#undef GUARD_OVERFLOW

// EOF

