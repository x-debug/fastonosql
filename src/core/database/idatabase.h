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

#include <string>

#include "core/connection_types.h"  // for connectionTypes
#include "core/core_fwd.h"
#include "core/icommand_translator.h"

#include "core/database/idatabase_info.h"

namespace fastonosql {
namespace core {
namespace events_info {
struct ClearDatabaseRequest;
}
}
}
namespace fastonosql {
namespace core {
namespace events_info {
struct ExecuteInfoRequest;
}
}
}
namespace fastonosql {
namespace core {
namespace events_info {
struct LoadDatabaseContentRequest;
}
}
}

namespace fastonosql {
namespace core {

class IDatabase {
 public:
  virtual ~IDatabase();

  IServerSPtr Server() const;
  IDataBaseInfoSPtr Info() const;
  translator_t Translator() const;

  connectionTypes Type() const;
  bool IsDefault() const;
  std::string Name() const;

  void LoadContent(const events_info::LoadDatabaseContentRequest& req);

  void Execute(const events_info::ExecuteInfoRequest& req);

 protected:
  IDatabase(IServerSPtr server, IDataBaseInfoSPtr info);

 private:
  const IDataBaseInfoSPtr info_;
  const IServerSPtr server_;
};

}  // namespace core
}  // namespace fastonosql
