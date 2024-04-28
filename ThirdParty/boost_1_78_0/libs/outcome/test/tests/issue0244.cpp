/* Unit testing for outcomes
(C) 2013-2021 Niall Douglas <http://www.nedproductions.biz/> (1 commit)


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <boost/outcome/result.hpp>
#include <boost/outcome/try.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_monitor.hpp>

namespace issues244
{
  namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

  static int counter = 0;
  static std::vector<int> default_constructor, copy_constructor, move_constructor, destructor;
  struct Foo
  {
    int x{2};
    explicit Foo(int v)
        : x(v)
    {
      std::cout << "   Default constructor " << ++counter << std::endl;
      default_constructor.push_back(counter);
    }
    Foo(const Foo &o) noexcept
        : x(o.x)
    {
      std::cout << "   Copy constructor " << ++counter << std::endl;
      copy_constructor.push_back(counter);
    }
    Foo(Foo &&o) noexcept
        : x(o.x)
    {
      std::cout << "   Move constructor " << ++counter << std::endl;
      move_constructor.push_back(counter);
    }
    ~Foo()
    {
      std::cout << "   Destructor " << ++counter << std::endl;
      destructor.push_back(counter);
    }
  };
  struct Immovable
  {
    int x{2};
    explicit Immovable(int v)
        : x(v)
    {
    }
      Immovable(const Immovable &) = delete;
    Immovable(Immovable &&) = delete;
  };

  outcome::result<Foo> get_foo() noexcept { return outcome::result<Foo>(outcome::in_place_type<Foo>, 5); }

  template <typename T> T &&filterR(T &&v) { return static_cast<T &&>(v); }
  template <typename T> const T &filterL(T &&v) { return v; }
}  // namespace issues244


BOOST_OUTCOME_AUTO_TEST_CASE(issues_0244_test, "TRY/TRYX has dangling reference if xvalues are emitted from tried expression")
{
  using namespace issues244;
  auto check = [](const char *desc, auto &&f) {
    counter = 0;
    default_constructor.clear();
    copy_constructor.clear();
    move_constructor.clear();
    destructor.clear();
    std::cout << "\n" << desc << std::endl;
    auto r = f();
    std::cout << "   Check integer " << ++counter << std::endl;
    BOOST_REQUIRE(r);
    BOOST_CHECK(r.value() == 5);
    if(!copy_constructor.empty())
    {
      BOOST_CHECK(copy_constructor.front() < destructor.front());
    }
    else if(!move_constructor.empty())
    {
      BOOST_CHECK(move_constructor.front() < destructor.front());
    }
  };

  /*
   Default constructor 1    (bind expression prvalue to unique rvalue)
   Move constructor 2       (move from unique rvalue to v value)
   After TRY 3
   Destructor 4             (destruct v value)
   Destructor 5             (destruct lifetime extended unique rvalue)
   Check integer 6
  */
  check("prvalue from expression with lifetime extension is moved into value", []() -> outcome::result<int> {
    BOOST_OUTCOME_TRY(auto v, get_foo());
    std::cout << "   After TRY " << ++counter << std::endl;
    return v.x;
  });
  /*
   Default constructor 1    (bind expression prvalue to unique rvalue)
                            (bind unique rvalue to v rvalue)
   After TRY 2
   Destructor 3             (destruct lifetime extended unique rvalue)
   Check integer 4
  */
  check("prvalue from expression with lifetime extension is bound into rvalue", []() -> outcome::result<int> {
    BOOST_OUTCOME_TRY(auto &&v, get_foo());
    std::cout << "   After TRY " << ++counter << std::endl;
    return v.x;
  });

  /*
   Default constructor 1
   Move constructor 2       (move expression xvalue into unique value)
   Destructor 3             (destruct expression xvalue)
   Move constructor 4       (move from unique value to v value)
   After TRY 5
   Destructor 6             (destruct v value)
   Destructor 7             (destruct unique value)
   Check integer 8
  */
  check("xvalue from expression without lifetime extension is moved into temporary and then moved into value", []() -> outcome::result<int> {
    BOOST_OUTCOME_TRY(auto v, filterR(get_foo()));
    std::cout << "   After TRY " << ++counter << std::endl;
    return v.x;
  });
  /*
   Default constructor 1
   Move constructor 2       (move expression xvalue into unique value)
   Destructor 3             (destruct expression xvalue)
   After TRY 4
   Destructor 5             (destruct unique value)
   Check integer 6
  */
  check("xvalue from expression without lifetime extension is moved into temporary and then bound into rvalue", []() -> outcome::result<int> {
    BOOST_OUTCOME_TRY(auto &&v, filterR(get_foo()));
    std::cout << "   After TRY " << ++counter << std::endl;
    return v.x;
  });

  /*
   Default constructor 1
   Copy constructor 2       (copy expression lvalue into unique value)
   Destructor 3             (destruct expression lvalue)
   Copy constructor 4       (copy from unique value to v value)
   After TRY 5
   Destructor 6             (destruct v value)
   Destructor 7             (destruct unique value)
   Check integer 8
  */
  check("lvalue from expression without lifetime extension is moved into temporary and then moved into value", []() -> outcome::result<int> {
    BOOST_OUTCOME_TRY(auto v, filterL(get_foo()));
    std::cout << "   After TRY " << ++counter << std::endl;
    return v.x;
  });
  /*
   Default constructor 1
   Copy constructor 2       (copy expression lvalue into unique value)
   Destructor 3             (destruct expression lvalue)
   After TRY 4
   Destructor 5             (destruct unique value)
   Check integer 6
  */
  check("lvalue from expression without lifetime extension is moved into temporary and then bound into rvalue", []() -> outcome::result<int> {
    BOOST_OUTCOME_TRY(auto &&v, filterL(get_foo()));
    std::cout << "   After TRY " << ++counter << std::endl;
    return v.x;
  });


  check("TRY lvalue passthrough", []() -> outcome::result<int> {
    const auto &x = outcome::result<Immovable>(outcome::in_place_type<Immovable>, 5);
    // Normally a lvalue input triggers value unique, which would fail to compile here
    BOOST_OUTCOME_TRY((auto &, v), x);
    return v.x;
  });

  // Force use of rvalue refs for unique and bound value
  check("TRY rvalue passthrough", []() -> outcome::result<int> {
    auto &&x = outcome::result<Immovable>(outcome::in_place_type<Immovable>, 5);
    // Normally an xvalue input triggers value unique, which would fail to compile here
    BOOST_OUTCOME_TRY((auto &&, v), x);
    return v.x;
  });

  // Force use of lvalue refs for unique and bound value
  check("TRY prvalue as lvalue passthrough", []() -> outcome::result<int> {
    outcome::result<Immovable> i(outcome::in_place_type<Immovable>, 5);
    BOOST_OUTCOME_TRY((auto &, v), i);
    return v.x;
  });

  // Force use of rvalue refs for unique and bound value
  check("TRY prvalue as rvalue passthrough", []() -> outcome::result<int> {
    outcome::result<Immovable> i(outcome::in_place_type<Immovable>, 5);
    BOOST_OUTCOME_TRY((auto &&, v), i);
    return v.x;
  });


  check("TRYV lvalue passthrough", []() -> outcome::result<int> {
    const auto &x = outcome::result<Immovable>(outcome::in_place_type<Immovable>, 5);
    // Normally a lvalue input triggers value unique, which would fail to compile here
    BOOST_OUTCOME_TRYV2(auto &, x);
    return 5;
  });

  // Force use of rvalue refs for unique and bound value
  check("TRYV rvalue passthrough", []() -> outcome::result<int> {
    auto &&x = outcome::result<Immovable>(outcome::in_place_type<Immovable>, 5);
    // Normally an xvalue input triggers value unique, which would fail to compile here
    BOOST_OUTCOME_TRYV2(auto &&, x);
    return 5;
  });

  // Force use of lvalue refs for unique and bound value
  check("TRYV prvalue as lvalue passthrough", []() -> outcome::result<int> {
    outcome::result<Immovable> i(outcome::in_place_type<Immovable>, 5);
    BOOST_OUTCOME_TRYV2(auto &, i);
    return 5;
  });

  // Force use of rvalue refs for unique and bound value
  check("TRYV prvalue as rvalue passthrough", []() -> outcome::result<int> {
    outcome::result<Immovable> i(outcome::in_place_type<Immovable>, 5);
    BOOST_OUTCOME_TRYV2(auto &, i);
    return 5;
  });
}
