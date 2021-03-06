/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    FastoNoSQL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoNoSQL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoNoSQL.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>  // for string

#include "core/config/config.h"

#define UNQLITE_DEFAULT_ENV_FLAGS 0x00000002  // unqlite Environment Flags
                                              // UNQLITE_OPEN_READWRITE        0x00000002

namespace fastonosql {
namespace core {
namespace unqlite {

struct Config : public LocalConfig {
  Config();
  bool ReadOnlyDB() const;
  void SetReadOnlyDB(bool ro);

  bool ReadWriteDB() const;
  void SetReadWriteDB(bool ro);

  bool CreateIfMissingDB() const;
  void SetCreateIfMissingDB(bool ro);

  int env_flags;
};

}  // namespace unqlite
}  // namespace core
}  // namespace fastonosql

namespace common {
std::string ConvertToString(const fastonosql::core::unqlite::Config& conf);
bool ConvertFromString(const std::string& from, fastonosql::core::unqlite::Config* out);
}
