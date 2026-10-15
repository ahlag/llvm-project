// RUN: split-file %s %t
// RUN: %clang_cc1 -fopenmp -fopenmp-version=52 \
// RUN:   -triple x86_64-unknown-linux -include %t/conditions.h \
// RUN:   %t/use.cpp -emit-llvm -o - | FileCheck %s
// RUN: %clang_cc1 -fopenmp -fopenmp-version=52 \
// RUN:   -triple x86_64-unknown-linux -x c++-header %t/conditions.h \
// RUN:   -emit-pch -o %t/conditions.pch
// RUN: %clang_cc1 -fopenmp -fopenmp-version=52 \
// RUN:   -triple x86_64-unknown-linux -include-pch %t/conditions.pch \
// RUN:   %t/use.cpp -emit-llvm -o - | FileCheck %s

// Conditions naming different parameters remain distinct even when their
// arguments have the same value. Exercise an attribute instantiated before
// serialization as well as attributes instantiated after loading the PCH.
// CHECK-LABEL: define{{.*}} i32 @instantiated_before_pch()
// CHECK: call i32 @high()
// CHECK: ret i32
// CHECK-LABEL: define{{.*}} i32 @distinct_parameters()
// CHECK: call i32 @high()
// CHECK: call i32 @high()
// CHECK: ret i32

// Parameters at the same position of different templates are different
// declarations.
// CHECK-LABEL: define{{.*}} i32 @different_templates()
// CHECK: call i32 @high()
// CHECK: call i32 @high()
// CHECK: call i32 @high()
// CHECK: call i32 @high()
// CHECK: call i32 @high()
// CHECK: ret i32

// Parameter names do not affect identity when their positions agree.
// CHECK-LABEL: define{{.*}} i32 @renamed_parameters()
// CHECK: call i32 @low()
// CHECK: call i32 @low()
// CHECK: call i32 @low()
// CHECK: ret i32

// Equal-valued enumerators in different scopes are different declarations.
// CHECK-LABEL: define{{.*}} i32 @different_scopes()
// CHECK: call i32 @high()
// CHECK: ret i32

// Compare a condition loaded from a PCH with one parsed in this process.
// They name the same declaration and must still form a subset relationship.
// CHECK-LABEL: define{{.*}} i32 @same_declaration()
// CHECK: call i32 @low()
// CHECK: ret i32

//--- conditions.h
extern "C" int high();
extern "C" int low();

#pragma omp declare variant(high) \
    match(user = {condition(N)}, \
          implementation = {vendor(score(100) : llvm)})
template <int N, int M> int distinct();
#pragma omp declare variant(low) \
    match(user = {condition(N)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
template <int M, int N> int distinct();

extern "C" int instantiated_before_pch() { return distinct<1, 1>(); }

#pragma omp declare variant(high) \
    match(user = {condition(N)}, \
          implementation = {vendor(score(100) : llvm)})
template <int N> int renamed();
#pragma omp declare variant(low) \
    match(user = {condition(A)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
template <int A> int renamed();

#pragma omp declare variant(high) \
    match(user = {condition(sizeof(T) > 1)}, \
          implementation = {vendor(score(100) : llvm)})
template <typename T> int renamed_type();
#pragma omp declare variant(low) \
    match(user = {condition(sizeof(U) > 1)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
template <typename U> int renamed_type();

template <typename> struct Box {
  static constexpr bool value = true;
};
#pragma omp declare variant(high) \
    match(user = {condition(TT<int>::value)}, \
          implementation = {vendor(score(100) : llvm)})
template <template <typename> class TT> int renamed_template();
#pragma omp declare variant(low) \
    match(user = {condition(UU<int>::value)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
template <template <typename> class UU> int renamed_template();

template <int N> struct NonTypeHigh {
#pragma omp declare variant(high) \
    match(user = {condition(N)}, \
          implementation = {vendor(score(100) : llvm)})
  friend int non_type_owner();
};
template <int N> struct NonTypeLow {
#pragma omp declare variant(low) \
    match(user = {condition(N)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
  friend int non_type_owner();
};

template <typename T> struct TypeHigh {
#pragma omp declare variant(high) \
    match(user = {condition(sizeof(T) > 1)}, \
          implementation = {vendor(score(100) : llvm)})
  friend int type_owner();
};
template <typename T> struct TypeLow {
#pragma omp declare variant(low) \
    match(user = {condition(sizeof(T) > 1)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
  friend int type_owner();
};

template <template <typename> class TT> struct TemplateHigh {
#pragma omp declare variant(high) \
    match(user = {condition(TT<int>::value)}, \
          implementation = {vendor(score(100) : llvm)})
  friend int template_owner();
};
template <template <typename> class TT> struct TemplateLow {
#pragma omp declare variant(low) \
    match(user = {condition(TT<int>::value)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
  friend int template_owner();
};

template <int... Ns> struct PackHigh {
#pragma omp declare variant(high) \
    match(user = {condition(sizeof...(Ns) > 0)}, \
          implementation = {vendor(score(100) : llvm)})
  friend int pack_owner();
};
template <int... Ns> struct PackLow {
#pragma omp declare variant(low) \
    match(user = {condition(sizeof...(Ns) > 0)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
  friend int pack_owner();
};

template <typename T> struct TypedefHigh {
  using U = T;
#pragma omp declare variant(high) \
    match(user = {condition(sizeof(U) > 1)}, \
          implementation = {vendor(score(100) : llvm)})
  friend int typedef_owner();
};
template <typename T> struct TypedefLow {
  using U = T;
#pragma omp declare variant(low) \
    match(user = {condition(sizeof(U) > 1)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
  friend int typedef_owner();
};

namespace First {
enum { A = 1 };
extern "C" {
#pragma omp declare variant(high) \
    match(user = {condition(A)}, \
          implementation = {vendor(score(100) : llvm)})
int scoped_base();
}
}

enum { Shared = 1 };
#pragma omp declare variant(high) \
    match(user = {condition(Shared)}, \
          implementation = {vendor(score(100) : llvm)})
int shared_base();

//--- use.cpp
namespace Second {
enum { A = 1 };
extern "C" {
#pragma omp declare variant(low) \
    match(user = {condition(A)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
int scoped_base();
}
}

#pragma omp declare variant(low) \
    match(user = {condition(Shared)}, device = {kind(cpu)}, \
          implementation = {vendor(score(1) : llvm)})
int shared_base();

extern "C" int distinct_parameters() {
  return distinct<1, 2>() + distinct<1, 1>();
}

template struct NonTypeHigh<1>;
template struct NonTypeLow<1>;
template struct TypeHigh<int>;
template struct TypeLow<int>;
template struct TemplateHigh<Box>;
template struct TemplateLow<Box>;
template struct PackHigh<1>;
template struct PackLow<1>;
template struct TypedefHigh<int>;
template struct TypedefLow<int>;
int non_type_owner();
int type_owner();
int template_owner();
int pack_owner();
int typedef_owner();

extern "C" int different_templates() {
  return non_type_owner() + type_owner() + template_owner() + pack_owner() +
         typedef_owner();
}

extern "C" int renamed_parameters() {
  return renamed<1>() + renamed_type<int>() + renamed_template<Box>();
}

extern "C" int different_scopes() {
  return Second::scoped_base();
}

extern "C" int same_declaration() {
  return shared_base();
}
