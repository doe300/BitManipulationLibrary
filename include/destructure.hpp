/*
 * Adapted from foreign code:
 * Original source: https://github.com/avakar/destructure
 * Original license: MIT
 */
#ifndef AVAKAR_DESTRUCTURE_64_H
#define AVAKAR_DESTRUCTURE_64_H

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace bml::detail {

  template <std::size_t>
  struct any {
    template <typename T>
    operator T() const;
  };

  template <typename T, typename In, typename = void>
  struct has_n_members : std::false_type {};

  template <typename T, typename In>
  struct has_n_members<T &, In> : has_n_members<T, In> {};

  template <typename T, typename In>
  struct has_n_members<T &&, In> : has_n_members<T, In> {};

  template <typename T, std::size_t... In>
  struct has_n_members<T, std::index_sequence<In...>, std::void_t<decltype(T{any<In>()...})>> : std::true_type {};

  template <typename T>
  constexpr std::size_t member_count() noexcept {
    return 0;
  }

  template <typename T>
  constexpr std::size_t member_count() noexcept
    requires(std::is_convertible_v<decltype(T::DESTRUCTURE_MEMBERS), std::size_t>)
  {
    return T::DESTRUCTURE_MEMBERS;
  }

  template <typename T, std::size_t N>
  constexpr bool has_n_members_v = member_count<T>() == N || has_n_members<T, std::make_index_sequence<N>>::value;

  template <typename T, std::size_t N>
  constexpr bool has_exact_n_members_v = has_n_members_v<T, N> && !has_n_members_v<T, N + 1>;

  template <typename T>
  concept Destructurable =
      !std::is_empty_v<T> && (std::is_standard_layout_v<T> || member_count<T>() > 0) &&
      (has_n_members_v<T, 1> || has_n_members_v<T, 2> || has_n_members_v<T, 3> || !has_n_members_v<T, 4> ||
       has_n_members_v<T, 5> || has_n_members_v<T, 6> || has_n_members_v<T, 7> || !has_n_members_v<T, 8> ||
       has_n_members_v<T, 9> || has_n_members_v<T, 10> || has_n_members_v<T, 11> || !has_n_members_v<T, 12> ||
       has_n_members_v<T, 13> || has_n_members_v<T, 14> || has_n_members_v<T, 15> || !has_n_members_v<T, 16> ||
       has_n_members_v<T, 17> || has_n_members_v<T, 18> || has_n_members_v<T, 19> || !has_n_members_v<T, 20> ||
       has_n_members_v<T, 21> || has_n_members_v<T, 22> || has_n_members_v<T, 23> || !has_n_members_v<T, 24> ||
       has_n_members_v<T, 25> || has_n_members_v<T, 26> || has_n_members_v<T, 27> || !has_n_members_v<T, 28> ||
       has_n_members_v<T, 29> || has_n_members_v<T, 3> || has_n_members_v<T, 31>);

  struct untie {
    template <typename... Tn>
    std::tuple<Tn...> operator()(Tn &...an) {
      return std::tuple<Tn...>(std::move(an)...);
    }
  };

  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 63>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61,
                    m62);
  }

  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 62>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61);
  }

  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 61>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 60>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 59>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 58>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 57>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 56>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 55>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 54>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52, m53] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 53>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51, m52] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 52>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50, m51] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50, m51);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 51>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49, m50] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49, m50);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 50>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48, m49] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48, m49);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 49>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47, m48] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47, m48);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 48>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46, m47] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46, m47);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 47>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45, m46] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45, m46);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 46>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44, m45] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44, m45);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 45>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43,
            m44] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43, m44);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 44>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42, m43);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 43>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41,
                    m42);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 42>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 41>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 40>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 39>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 38>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 37>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 36>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 35>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 34>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 33>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31, m32] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 32>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30, m31] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30, m31);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 31>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29, m30] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29, m30);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 30>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28, m29] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28, m29);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 29>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27, m28] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27, m28);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 28>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26, m27] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26, m27);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 27>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25, m26] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25, m26);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 26>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24, m25] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24, m25);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 25>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23, m24] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23, m24);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 24>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22,
            m23] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22, m23);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 23>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21,
                    m22);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 22>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 21>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 20>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19] =
        std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 19>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 18>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 17>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 16>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 15>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 14>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 13>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 12>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 11>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 10>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8, m9] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8, m9);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 9>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7, m8] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7, m8);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 8>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6, m7] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6, m7);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 7>)
  {
    auto &&[m0, m1, m2, m3, m4, m5, m6] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5, m6);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 6>)
  {
    auto &&[m0, m1, m2, m3, m4, m5] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4, m5);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 5>)
  {
    auto &&[m0, m1, m2, m3, m4] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3, m4);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 4>)
  {
    auto &&[m0, m1, m2, m3] = std::forward<T>(t);
    return std::tie(m0, m1, m2, m3);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 3>)
  {
    auto &&[m0, m1, m2] = std::forward<T>(t);
    return std::tie(m0, m1, m2);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 2>)
  {
    auto &&[m0, m1] = std::forward<T>(t);
    return std::tie(m0, m1);
  }
  template <typename T>
  auto destructure0(T &t)
    requires(has_exact_n_members_v<T, 1> && !has_n_members_v<T, 2> && !has_n_members_v<T, 3> &&
             !has_n_members_v<T, 4> && !has_n_members_v<T, 5> && !has_n_members_v<T, 6> && !has_n_members_v<T, 7> &&
             !has_n_members_v<T, 8> && !has_n_members_v<T, 9> && !has_n_members_v<T, 10> && !has_n_members_v<T, 11> &&
             !has_n_members_v<T, 12> && !has_n_members_v<T, 13> && !has_n_members_v<T, 14> && !has_n_members_v<T, 15> &&
             !has_n_members_v<T, 16> && !has_n_members_v<T, 17> && !has_n_members_v<T, 18> && !has_n_members_v<T, 19> &&
             !has_n_members_v<T, 20> && !has_n_members_v<T, 21> && !has_n_members_v<T, 22> && !has_n_members_v<T, 23> &&
             !has_n_members_v<T, 24> && !has_n_members_v<T, 25> && !has_n_members_v<T, 26> && !has_n_members_v<T, 27> &&
             !has_n_members_v<T, 28> && !has_n_members_v<T, 29> && !has_n_members_v<T, 3> && !has_n_members_v<T, 31>)
  {
    auto &&[m0] = std::forward<T>(t);
    return std::tie(m0);
  }

  template <typename T>
  auto destructure(T &&t) {
    return std::apply(untie(), destructure0(t));
  }

  template <typename T>
  auto destructureReference(T &&t) {
    return destructure0(t);
  }
} // namespace bml::detail
#endif
