#include "stdafx.h"

#include <iostream>
#include <typeinfo>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/integral_constant.hpp>

struct HavingFoo { int foo; };
template <typename T> struct FooCollisionEnforcer : T, HavingFoo { };

template <typename T, int HavingFoo::* = &FooCollisionEnforcer<T>::foo> struct NoFooCollision { typedef void type; };

template <typename T, typename=void> struct IfAnyFooEnabled { typedef void type; };
template <typename T> struct IfAnyFooEnabled<T, typename NoFooCollision<T>::type> { };

template <typename T, typename=void> struct IsFooEnabled : boost::false_type { };
template <typename T> struct IsFooEnabled<T, typename IfAnyFooEnabled<T>::type>
{
   typedef char(&yes)[1];
   typedef char(&no)[2];
   template <typename C> static yes checkFooSignature(void(C::*)(int) const);
   static no checkFooSignature(...);

   static const bool value = sizeof(checkFooSignature(&T::foo)) == sizeof(yes);
};
template <typename T> struct IfFooEnabled : boost::enable_if<IsFooEnabled<T> > { };
template <typename T> struct IfFooDisabled : boost::disable_if<IsFooEnabled<T> > { };

class TestBinding
{
public:
   void foo(int value) const { std::cout << "foo has been called with " << value << std::endl; }
};

class TestBindingNoFoo
{
public:
};

template<class T>
class Accessor : public T
{
};

typedef void FooSignature(int);

template<class T>
class TestFoo : public T
{
private:

   template <typename U>
   typename IfFooEnabled<U>::type doTest()
   {
      std::cout << "foo is enabled" << std::endl;
      T::foo(42); // non-virtual call
      this->foo(123);
   }

   template <typename U>
   typename IfFooDisabled<U>::type doTest()
   {
      std::cout << "foo is disabled" << std::endl;
      // T::foo(42); // error: 'foo' is not a member of 'Accessor<TestBindingNoFoo>'
      // this->foo(123); // error: 'foo' is not a member of 'Accessor<TestBindingNoFoo>'
   }

public:
   TestFoo()
   {
      doTest<T>();
      doTest<TestFoo>();
   }
};

int main(int argc, char* argv[])
{
   typedef TestFoo<Accessor<TestBinding> > TestFoo1;
   typedef TestFoo<Accessor<TestBindingNoFoo> > TestFoo2;

   TestFoo1 obj1;
   TestFoo2 obj2;
}

