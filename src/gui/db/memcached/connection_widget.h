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

#include "gui/widgets/connection_remote_widget.h"

namespace fastonosql {
namespace gui {
class UserPasswordWidget;
namespace memcached {

class ConnectionWidget : public ConnectionRemoteWidget {
  Q_OBJECT
 public:
  explicit ConnectionWidget(QWidget* parent = 0);

  virtual void syncControls(proxy::IConnectionSettingsBase* connection) override;
  virtual void retranslateUi() override;
  virtual bool validated() const override;
  bool isValidCredential() const;

 private Q_SLOTS:
  void saslStateChange(int state);

 private:
  virtual proxy::IConnectionSettingsRemote* createConnectionRemoteImpl(
      const proxy::connection_path_t& path) const override;

  QCheckBox* useSasl_;
  UserPasswordWidget* userPasswordWidget_;
};

}  // namespace memcached
}  // namespace gui
}  // namespace fastonosql
