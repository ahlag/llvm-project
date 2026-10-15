// RUN: %clang_cc1 -fopenmp -fopenmp-version=52 -triple x86_64-unknown-linux \
// RUN:   -emit-llvm %s -o - | FileCheck %s

// A lambda parameter and a parameter of its enclosing function share a
// position but are different declarations.
// CHECK-LABEL: define{{.*}}distinct_functionsi{{.*}}clEi"(
// CHECK-NOT: __kmpc_omp_taskwait
// CHECK: call {{.*}} @__kmpc_omp_taskyield(
// CHECK-NOT: __kmpc_omp_taskwait
// CHECK: ret void
void distinct_functions(int a) {
  auto l = [&](int b) {
#pragma omp metadirective \
    when(implementation = {vendor(score(100) : llvm), extension(match_any)}, \
         user = {condition(a > 0)} : taskyield) \
    when(implementation = {vendor(llvm), extension(match_any)}, \
         device = {kind(cpu)}, user = {condition(b > 0)} : taskwait)
  };
  l(1);
}

// CHECK-LABEL: define{{.*}}same_parameteri{{.*}}clEi"(
// CHECK-NOT: __kmpc_omp_taskyield
// CHECK: call {{.*}} @__kmpc_omp_taskwait(
// CHECK-NOT: __kmpc_omp_taskyield
// CHECK: ret void
void same_parameter(int a) {
  auto l = [&](int b) {
#pragma omp metadirective \
    when(implementation = {vendor(score(100) : llvm), extension(match_any)}, \
         user = {condition(b > 0)} : taskyield) \
    when(implementation = {vendor(llvm), extension(match_any)}, \
         device = {kind(cpu)}, user = {condition(b > 0)} : taskwait)
  };
  l(1);
}

// A non-constant score after the user condition must not replace its identity.
// CHECK-LABEL: define{{.*}} void @non_constant_score(
// CHECK-NOT: __kmpc_omp_taskwait
// CHECK: call {{.*}} @__kmpc_omp_taskyield(
// CHECK-NOT: __kmpc_omp_taskwait
// CHECK: ret void
extern "C" void non_constant_score(int n) {
#pragma omp metadirective \
    when(user = {condition(1)}, \
         implementation = {vendor(score(100) : llvm), \
                           extension(score(n) : match_any)} : taskyield) \
    when(user = {condition(2)}, \
         implementation = {vendor(llvm), extension(score(n) : match_any)}, \
         device = {kind(cpu)} : taskwait)
}

typedef float F2 __attribute__((ext_vector_type(2)));
typedef int I2 __attribute__((ext_vector_type(2)));
typedef long L2 __attribute__((ext_vector_type(2)));

// Value initializations and vector conversions to different types differ.
// CHECK-LABEL: define{{.*}} void @written_types(
// CHECK-NOT: __kmpc_omp_taskwait
// CHECK: call {{.*}} @__kmpc_omp_taskyield(
// CHECK: call {{.*}} @__kmpc_omp_taskyield(
// CHECK-NOT: __kmpc_omp_taskwait
// CHECK: ret void
extern "C" void written_types(F2 v) {
#pragma omp metadirective \
    when(user = {condition(!int())}, \
         implementation = {vendor(score(100) : llvm)} : taskyield) \
    when(user = {condition(!long())}, device = {kind(cpu)}, \
         implementation = {vendor(score(1) : llvm)} : taskwait)
#pragma omp metadirective \
    when(user = {condition(sizeof(__builtin_convertvector(v, I2)) > 0)}, \
         implementation = {vendor(score(100) : llvm)} : taskyield) \
    when(user = {condition(sizeof(__builtin_convertvector(v, L2)) > 0)}, \
         device = {kind(cpu)}, \
         implementation = {vendor(score(1) : llvm)} : taskwait)
}
