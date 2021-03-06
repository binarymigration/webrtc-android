// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_UTIL_TYPE_SAFETY_TOKEN_TYPE_H_
#define BASE_UTIL_TYPE_SAFETY_TOKEN_TYPE_H_

#include <type_traits>

#include "base/types/strong_alias.h"
#include "base/unguessable_token.h"

namespace util {

// A specialization of StrongAlias for base::UnguessableToken. Unlike
// base::UnguessableToken, a TokenType<...> does not default to null and does
// not expose the concept of null tokens. If you need to indicate a null token,
// please use base::Optional<TokenType<...>>.
template <typename TypeMarker,
          // Please do not use this unless absolutely necessary! It is only
          // present in support of ongoing migrations. See crbug.com/1096617.
          bool kAllowImplicitConversion = false>
class TokenType : public base::StrongAlias<TypeMarker, base::UnguessableToken> {
 private:
  using Super = base::StrongAlias<TypeMarker, base::UnguessableToken>;

 public:
  TokenType() : Super(base::UnguessableToken::Create()) {}
  // The parameter |unused| is here to prevent multiple definitions of a
  // single argument constructor. This is only needed during the migration to
  // strongly typed frame tokens.
  // TODO(crbug.com/1096617): Remove this after the token type migration.
  explicit TokenType(const base::UnguessableToken& token, int unused = 0)
      : Super(token) {}
  TokenType(const TokenType& token) : Super(token.value()) {}
  TokenType(TokenType&& token) noexcept : Super(token.value()) {}
  TokenType& operator=(const TokenType& token) = default;
  TokenType& operator=(TokenType&& token) noexcept = default;

  // Allow implicit promotion from base::UnguessableToken if specified.
  // TODO(crbug.com/1096617): Remove this after the token type migration.
  template <bool kAllowImplicitConversion2 = kAllowImplicitConversion,
            typename = std::enable_if_t<kAllowImplicitConversion2>>
  // NOLINTNEXTLINE(google-explicit-constructor)
  TokenType(const base::UnguessableToken& token) : Super(token) {}

  // This object allows default assignment operators for compatibility with
  // STL containers.

  // Hash functor for use in unordered containers.
  struct Hasher {
    using argument_type = TokenType;
    using result_type = size_t;
    result_type operator()(const argument_type& token) const {
      return base::UnguessableTokenHash()(token.value());
    }
  };

  // Mimic the base::UnguessableToken API for ease and familiarity of use.
  std::string ToString() const { return this->value().ToString(); }

  // Allow implicit conversion down to a base::UnguessableToken, if desired.
  // This is only present during the strongly-typed token migration.
  // TODO(crbug.com/1096617): Remove this after the token type migration.
  template <bool kAllowImplicitConversion2 = kAllowImplicitConversion,
            typename = std::enable_if_t<kAllowImplicitConversion2>>
  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr operator const base::UnguessableToken&() const& {
    return this->value();
  }
};

}  // namespace util

#endif  // BASE_UTIL_TYPE_SAFETY_TOKEN_TYPE_H_
