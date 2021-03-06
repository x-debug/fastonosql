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

#include "proxy/server/iserver.h"

#include <stddef.h>  // for size_t
#include <string>    // for string, operator==, etc

#include <QApplication>

#include <common/error.h>        // for Error
#include <common/macros.h>       // for VERIFY, CHECK, DNOTREACHED
#include <common/value.h>        // for ErrorValue
#include <common/qt/utils_qt.h>  // for Event<>::value_type
#include <common/qt/logger.h>    // for LOG_ERROR

#include "proxy/connection_settings/iconnection_settings.h"
#include "proxy/events/events_info.h"  // for LoadDatabaseContentResponce, etc
#include "proxy/driver/idriver.h"      // for IDriver

namespace fastonosql {
namespace proxy {

IServer::IServer(IDriver* drv)
    : drv_(drv), server_info_(), current_database_info_(), timer_check_key_exists_id_(0) {
  VERIFY(QObject::connect(drv_, &IDriver::ChildAdded, this, &IServer::ChildAdded));
  VERIFY(QObject::connect(drv_, &IDriver::ItemUpdated, this, &IServer::ItemUpdated));
  VERIFY(
      QObject::connect(drv_, &IDriver::ServerInfoSnapShoot, this, &IServer::ServerInfoSnapShoot));

  VERIFY(QObject::connect(drv_, &IDriver::FlushedDB, this, &IServer::FlushDB));
  VERIFY(QObject::connect(drv_, &IDriver::CurrentDataBaseChanged, this,
                          &IServer::CurrentDataBaseChange));
  VERIFY(QObject::connect(drv_, &IDriver::KeyRemoved, this, &IServer::KeyRemove));
  VERIFY(QObject::connect(drv_, &IDriver::KeyAdded, this, &IServer::KeyAdd));
  VERIFY(QObject::connect(drv_, &IDriver::KeyLoaded, this, &IServer::KeyLoad));
  VERIFY(QObject::connect(drv_, &IDriver::KeyRenamed, this, &IServer::KeyRename));
  VERIFY(QObject::connect(drv_, &IDriver::KeyTTLChanged, this, &IServer::KeyTTLChange));
  VERIFY(QObject::connect(drv_, &IDriver::KeyTTLLoaded, this, &IServer::KeyTTLLoad));
  VERIFY(QObject::connect(drv_, &IDriver::Disconnected, this, &IServer::Disconnected));

  drv_->Start();
}

IServer::~IServer() {
  StopCurrentEvent();
  drv_->Stop();
  delete drv_;
}

void IServer::StartCheckKeyExistTimer() {
  timer_check_key_exists_id_ = startTimer(1000);
  DCHECK(timer_check_key_exists_id_ != 0);
}

void IServer::StopCheckKeyExistTimer() {
  if (timer_check_key_exists_id_ != 0) {
    killTimer(timer_check_key_exists_id_);
    timer_check_key_exists_id_ = 0;
  }
}

void IServer::StopCurrentEvent() {
  drv_->Interrupt();
}

bool IServer::IsConnected() const {
  return drv_->IsConnected() && drv_->IsAuthenticated();
}

bool IServer::IsCanRemote() const {
  return IsRemoteType(Type());
}

bool IServer::IsSupportTTLKeys() const {
  return fastonosql::core::IsSupportTTLKeys(Type());
}

core::translator_t IServer::Translator() const {
  return drv_->Translator();
}

core::connectionTypes IServer::Type() const {
  return drv_->Type();
}

std::string IServer::Name() const {
  connection_path_t path = drv_->ConnectionPath();
  return path.Name();
}

core::IServerInfoSPtr IServer::CurrentServerInfo() const {
  if (IsConnected()) {
    DCHECK(server_info_);
    return server_info_;
  }

  return core::IServerInfoSPtr();
}

IServer::database_t IServer::CurrentDatabaseInfo() const {
  if (IsConnected()) {
    return current_database_info_;
  }

  return database_t();
}

std::string IServer::Delimiter() const {
  return drv_->Delimiter();
}

std::string IServer::NsSeparator() const {
  return drv_->NsSeparator();
}

IDatabaseSPtr IServer::CreateDatabaseByInfo(core::IDataBaseInfoSPtr inf) {
  database_t db = FindDatabase(inf);
  return db ? CreateDatabase(inf) : IDatabaseSPtr();
}

proxy::IServer::database_t IServer::FindDatabase(core::IDataBaseInfoSPtr inf) const {
  if (!inf) {
    DNOTREACHED();
    return database_t();
  }

  CHECK(Type() == inf->Type());
  for (database_t db : databases_) {
    if (db->Name() == inf->Name()) {
      return db;
    }
  }

  return database_t();
}

void IServer::Connect(const events_info::ConnectInfoRequest& req) {
  emit ConnectStarted(req);
  QEvent* ev = new events::ConnectRequestEvent(this, req);
  Notify(ev);
}

void IServer::Disconnect(const events_info::DisConnectInfoRequest& req) {
  StopCurrentEvent();
  emit DisconnectStarted(req);
  QEvent* ev = new events::DisconnectRequestEvent(this, req);
  Notify(ev);
}

void IServer::LoadDatabases(const events_info::LoadDatabasesInfoRequest& req) {
  emit LoadDatabasesStarted(req);
  QEvent* ev = new events::LoadDatabasesInfoRequestEvent(this, req);
  Notify(ev);
}

void IServer::LoadDatabaseContent(const events_info::LoadDatabaseContentRequest& req) {
  emit LoadDataBaseContentStarted(req);
  QEvent* ev = new events::LoadDatabaseContentRequestEvent(this, req);
  Notify(ev);
}

void IServer::Execute(const events_info::ExecuteInfoRequest& req) {
  emit ExecuteStarted(req);
  QEvent* ev = new events::ExecuteRequestEvent(this, req);
  Notify(ev);
}

void IServer::ShutDown(const events_info::ShutDownInfoRequest& req) {
  emit ShutdownStarted(req);
  QEvent* ev = new events::ShutDownRequestEvent(this, req);
  Notify(ev);
}

void IServer::BackupToPath(const events_info::BackupInfoRequest& req) {
  emit BackupStarted(req);
  QEvent* ev = new events::BackupRequestEvent(this, req);
  Notify(ev);
}

void IServer::ExportFromPath(const events_info::ExportInfoRequest& req) {
  emit ExportStarted(req);
  QEvent* ev = new events::ExportRequestEvent(this, req);
  Notify(ev);
}

void IServer::ChangePassword(const events_info::ChangePasswordRequest& req) {
  emit ChangePasswordStarted(req);
  QEvent* ev = new events::ChangePasswordRequestEvent(this, req);
  Notify(ev);
}

void IServer::SetMaxConnection(const events_info::ChangeMaxConnectionRequest& req) {
  emit ChangeMaxConnectionStarted(req);
  QEvent* ev = new events::ChangeMaxConnectionRequestEvent(this, req);
  Notify(ev);
}

void IServer::LoadServerInfo(const events_info::ServerInfoRequest& req) {
  emit LoadServerInfoStarted(req);
  QEvent* ev = new events::ServerInfoRequestEvent(this, req);
  Notify(ev);
}

void IServer::ServerProperty(const events_info::ServerPropertyInfoRequest& req) {
  emit LoadServerPropertyStarted(req);
  QEvent* ev = new events::ServerPropertyInfoRequestEvent(this, req);
  Notify(ev);
}

void IServer::RequestHistoryInfo(const events_info::ServerInfoHistoryRequest& req) {
  emit LoadServerHistoryInfoStarted(req);
  QEvent* ev = new events::ServerInfoHistoryRequestEvent(this, req);
  Notify(ev);
}

void IServer::ClearHistory(const events_info::ClearServerHistoryRequest& req) {
  emit ClearServerHistoryStarted(req);
  QEvent* ev = new events::ClearServerHistoryRequestEvent(this, req);
  Notify(ev);
}

void IServer::ChangeProperty(const events_info::ChangeServerPropertyInfoRequest& req) {
  emit ChangeServerPropertyStarted(req);
  QEvent* ev = new events::ChangeServerPropertyInfoRequestEvent(this, req);
  Notify(ev);
}

void IServer::LoadChannels(const events_info::LoadServerChannelsRequest& req) {
  emit LoadServerChannelsStarted(req);
  QEvent* ev = new events::LoadServerChannelsRequestEvent(this, req);
  Notify(ev);
}

void IServer::customEvent(QEvent* event) {
  QEvent::Type type = event->type();
  if (type == static_cast<QEvent::Type>(events::ConnectResponceEvent::EventType)) {
    events::ConnectResponceEvent* ev = static_cast<events::ConnectResponceEvent*>(event);
    HandleConnectEvent(ev);

    events::ConnectResponceEvent::value_type v = ev->value();
    common::Error er(v.errorInfo());
    if (!er) {
      events_info::DiscoveryInfoRequest dreq(this);
      ProcessDiscoveryInfo(dreq);
    }
  } else if (type == static_cast<QEvent::Type>(events::EnterModeEvent::EventType)) {
    events::EnterModeEvent* ev = static_cast<events::EnterModeEvent*>(event);
    HandleEnterModeEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::LeaveModeEvent::EventType)) {
    events::LeaveModeEvent* ev = static_cast<events::LeaveModeEvent*>(event);
    HandleLeaveModeEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::CommandRootCreatedEvent::EventType)) {
    events::CommandRootCreatedEvent* ev = static_cast<events::CommandRootCreatedEvent*>(event);
    events::CommandRootCreatedEvent::value_type v = ev->value();
    emit RootCreated(v);
  } else if (type == static_cast<QEvent::Type>(events::CommandRootCompleatedEvent::EventType)) {
    events::CommandRootCompleatedEvent* ev =
        static_cast<events::CommandRootCompleatedEvent*>(event);
    events::CommandRootCompleatedEvent::value_type v = ev->value();
    emit RootCompleated(v);
  } else if (type == static_cast<QEvent::Type>(events::DisconnectResponceEvent::EventType)) {
    events::DisconnectResponceEvent* ev = static_cast<events::DisconnectResponceEvent*>(event);
    HandleDisconnectEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::LoadDatabasesInfoResponceEvent::EventType)) {
    events::LoadDatabasesInfoResponceEvent* ev =
        static_cast<events::LoadDatabasesInfoResponceEvent*>(event);
    HandleLoadDatabaseInfosEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::ServerInfoResponceEvent::EventType)) {
    events::ServerInfoResponceEvent* ev = static_cast<events::ServerInfoResponceEvent*>(event);
    HandleLoadServerInfoEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::ServerInfoHistoryResponceEvent::EventType)) {
    events::ServerInfoHistoryResponceEvent* ev =
        static_cast<events::ServerInfoHistoryResponceEvent*>(event);
    HandleLoadServerInfoHistoryEvent(ev);
  } else if (type ==
             static_cast<QEvent::Type>(events::ClearServerHistoryResponceEvent::EventType)) {
    events::ClearServerHistoryResponceEvent* ev =
        static_cast<events::ClearServerHistoryResponceEvent*>(event);
    HandleClearServerHistoryResponceEvent(ev);
  } else if (type ==
             static_cast<QEvent::Type>(events::ServerPropertyInfoResponceEvent::EventType)) {
    events::ServerPropertyInfoResponceEvent* ev =
        static_cast<events::ServerPropertyInfoResponceEvent*>(event);
    HandleLoadServerPropertyEvent(ev);
  } else if (type ==
             static_cast<QEvent::Type>(events::ChangeServerPropertyInfoResponceEvent::EventType)) {
    events::ChangeServerPropertyInfoResponceEvent* ev =
        static_cast<events::ChangeServerPropertyInfoResponceEvent*>(event);
    HandleServerPropertyChangeEvent(ev);
  } else if (type ==
             static_cast<QEvent::Type>(events::LoadServerChannelsResponceEvent::EventType)) {
    events::LoadServerChannelsResponceEvent* ev =
        static_cast<events::LoadServerChannelsResponceEvent*>(event);
    HandleLoadServerChannelsEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::BackupResponceEvent::EventType)) {
    events::BackupResponceEvent* ev = static_cast<events::BackupResponceEvent*>(event);
    HandleBackupEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::ExportResponceEvent::EventType)) {
    events::ExportResponceEvent* ev = static_cast<events::ExportResponceEvent*>(event);
    HandleExportEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::ChangePasswordResponceEvent::EventType)) {
    events::ChangePasswordResponceEvent* ev =
        static_cast<events::ChangePasswordResponceEvent*>(event);
    HandleChangePasswordEvent(ev);
  } else if (type ==
             static_cast<QEvent::Type>(events::ChangeMaxConnectionResponceEvent::EventType)) {
    events::ChangeMaxConnectionResponceEvent* ev =
        static_cast<events::ChangeMaxConnectionResponceEvent*>(event);
    HandleChangeMaxConnectionEvent(ev);
  } else if (type ==
             static_cast<QEvent::Type>(events::LoadDatabaseContentResponceEvent::EventType)) {
    events::LoadDatabaseContentResponceEvent* ev =
        static_cast<events::LoadDatabaseContentResponceEvent*>(event);
    HandleLoadDatabaseContentEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::ExecuteResponceEvent::EventType)) {
    events::ExecuteResponceEvent* ev = static_cast<events::ExecuteResponceEvent*>(event);
    HandleExecuteEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::DiscoveryInfoResponceEvent::EventType)) {
    events::DiscoveryInfoResponceEvent* ev =
        static_cast<events::DiscoveryInfoResponceEvent*>(event);
    HandleDiscoveryInfoResponceEvent(ev);
  } else if (type == static_cast<QEvent::Type>(events::ProgressResponceEvent::EventType)) {
    events::ProgressResponceEvent* ev = static_cast<events::ProgressResponceEvent*>(event);
    events::ProgressResponceEvent::value_type v = ev->value();
    emit ProgressChanged(v);
  }

  return QObject::customEvent(event);
}

void IServer::timerEvent(QTimerEvent* event) {
  if (timer_check_key_exists_id_ == event->timerId() && IsConnected()) {
    database_t cdb = CurrentDatabaseInfo();
    HandleCheckDBKeys(cdb, 1);
  }
  QObject::timerEvent(event);
}

void IServer::Notify(QEvent* ev) {
  events_info::ProgressInfoResponce resp(0);
  emit ProgressChanged(resp);
  qApp->postEvent(drv_, ev);
}

void IServer::HandleConnectEvent(events::ConnectResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }
  emit ConnectFinished(v);
}

void IServer::HandleDisconnectEvent(events::DisconnectResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }
  emit DisconnectFinished(v);
}

void IServer::HandleLoadServerInfoEvent(events::ServerInfoResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }
  emit LoadServerInfoFinished(v);
}

void IServer::HandleLoadServerPropertyEvent(events::ServerPropertyInfoResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }
  emit LoadServerPropertyFinished(v);
}

void IServer::HandleServerPropertyChangeEvent(events::ChangeServerPropertyInfoResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }
  emit ChangeServerPropertyFinished(v);
}

void IServer::HandleLoadServerChannelsEvent(events::LoadServerChannelsResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }
  emit LoadServerChannelsFinished(v);
}

void IServer::HandleShutdownEvent(events::ShutDownResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }
  emit ShutdownFinished(v);
}

void IServer::HandleBackupEvent(events::BackupResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }
  emit BackupFinished(v);
}

void IServer::HandleExportEvent(events::ExportResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }
  emit ExportFinished(v);
}

void IServer::HandleChangePasswordEvent(events::ChangePasswordResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }

  emit ChangePasswordFinished(v);
}

void IServer::HandleChangeMaxConnectionEvent(events::ChangeMaxConnectionResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }

  emit ChangeMaxConnectionFinished(v);
}

void IServer::HandleExecuteEvent(events::ExecuteResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }

  emit ExecuteFinished(v);
}

void IServer::HandleLoadDatabaseInfosEvent(events::LoadDatabasesInfoResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  } else {
    events_info::LoadDatabasesInfoResponce::database_info_cont_type dbs = v.databases;
    events_info::LoadDatabasesInfoResponce::database_info_cont_type tmp;
    for (size_t j = 0; j < dbs.size(); ++j) {
      core::IDataBaseInfoSPtr db = dbs[j];
      database_t dbs = FindDatabase(db);
      if (!dbs) {
        DCHECK(!db->IsDefault());
        databases_.push_back(db);
      }
      tmp.push_back(db);
    }
    v.databases = tmp;
  }

  emit LoadDatabasesFinished(v);
}

void IServer::HandleLoadDatabaseContentEvent(events::LoadDatabaseContentResponceEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  } else {
    database_t dbs = FindDatabase(v.inf);
    if (dbs) {
      dbs->SetKeys(v.keys);
      dbs->SetDBKeysCount(v.db_keys_count);
      v.inf = dbs;
    }
  }

  emit LoadDatabaseContentFinished(v);
}

void IServer::FlushDB() {
  database_t cdb = CurrentDatabaseInfo();
  if (!cdb) {
    return;
  }

  cdb->ClearKeys();
  cdb->SetDBKeysCount(0);
  emit FlushedDB(cdb);
}

void IServer::CurrentDataBaseChange(core::IDataBaseInfoSPtr db) {
  database_t cdb = CurrentDatabaseInfo();
  if (cdb) {
    if (db->Name() == cdb->Name()) {
      return;
    }
  }

  database_t founded;
  for (database_t cached_db : databases_) {
    if (db->Name() == cached_db->Name()) {
      founded = cached_db;
      founded->SetIsDefault(true);
    } else {
      cached_db->SetIsDefault(false);
    }
  }

  if (!founded) {
    founded = db;
    databases_.push_back(founded);
    current_database_info_ = founded;
  } else {
    current_database_info_ = founded;
  }

  DCHECK(founded->IsDefault());
  emit CurrentDataBaseChanged(founded);
}

void IServer::KeyRemove(core::NKey key) {
  database_t cdb = CurrentDatabaseInfo();
  if (!cdb) {
    return;
  }

  if (cdb->RemoveKey(key)) {
    emit KeyRemoved(cdb, key);
  }
}

void IServer::KeyAdd(core::NDbKValue key) {
  database_t cdb = CurrentDatabaseInfo();
  if (!cdb) {
    return;
  }

  if (cdb->InsertKey(key)) {
    emit KeyAdded(cdb, key);
  } else {
    emit KeyLoaded(cdb, key);
  }
}

void IServer::KeyLoad(core::NDbKValue key) {
  database_t cdb = CurrentDatabaseInfo();
  if (!cdb) {
    return;
  }

  if (cdb->InsertKey(key)) {
    emit KeyAdded(cdb, key);
  } else {
    emit KeyLoaded(cdb, key);
  }
}

void IServer::KeyRename(core::NKey key, std::string new_name) {
  database_t cdb = CurrentDatabaseInfo();
  if (!cdb) {
    return;
  }

  if (cdb->RenameKey(key, new_name)) {
    emit KeyRenamed(cdb, key, new_name);
  }
}

void IServer::KeyTTLChange(core::NKey key, core::ttl_t ttl) {
  database_t cdb = CurrentDatabaseInfo();
  if (!cdb) {
    return;
  }

  if (cdb->UpdateKeyTTL(key, ttl)) {
    emit KeyTTLChanged(cdb, key, ttl);
  }
}

void IServer::KeyTTLLoad(core::NKey key, core::ttl_t ttl) {
  database_t cdb = CurrentDatabaseInfo();
  if (!cdb) {
    return;
  }

  if (ttl == EXPIRED_TTL) {
    if (cdb->RemoveKey(key)) {
      emit KeyRemoved(cdb, key);
    }
    return;
  }

  if (cdb->UpdateKeyTTL(key, ttl)) {
    emit KeyTTLChanged(cdb, key, ttl);
  }
}

void IServer::HandleCheckDBKeys(core::IDataBaseInfoSPtr db, core::ttl_t expired_time) {
  if (!db) {
    return;
  }

  auto keys = db->Keys();
  for (core::NDbKValue key : keys) {
    core::NKey nkey = key.Key();
    core::ttl_t key_ttl = nkey.TTL();
    if (key_ttl == NO_TTL) {
    } else if (key_ttl == EXPIRED_TTL) {
      if (db->RemoveKey(nkey)) {
        emit KeyRemoved(db, nkey);
      }
    } else {  // live
      const core::ttl_t new_ttl = key_ttl - expired_time;
      if (new_ttl == NO_TTL) {
        core::translator_t trans = Translator();
        std::string load_ttl_cmd;
        common::Error err = trans->LoadKeyTTLCommand(nkey, &load_ttl_cmd);
        if (err && err->IsError()) {
          return;
        }
        proxy::events_info::ExecuteInfoRequest req(this, load_ttl_cmd, 0, 0, true, true,
                                                   core::C_INNER);
        Execute(req);
      } else {
        if (db->UpdateKeyTTL(nkey, new_ttl)) {
          emit KeyTTLChanged(db, nkey, new_ttl);
        }
      }
    }
  }
}

void IServer::HandleEnterModeEvent(events::EnterModeEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }

  emit ModeEntered(v);
}

void IServer::HandleLeaveModeEvent(events::LeaveModeEvent* ev) {
  auto v = ev->value();
  common::Error er(v.errorInfo());
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }

  emit ModeLeaved(v);
}

void IServer::HandleLoadServerInfoHistoryEvent(events::ServerInfoHistoryResponceEvent* ev) {
  auto v = ev->value();
  common::Error er = v.errorInfo();
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }

  emit LoadServerHistoryInfoFinished(v);
}

void IServer::HandleDiscoveryInfoResponceEvent(events::DiscoveryInfoResponceEvent* ev) {
  auto v = ev->value();
  common::Error er = v.errorInfo();
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  } else {
    server_info_ = v.sinfo;
    database_t dbs = FindDatabase(v.dbinfo);
    if (!dbs) {
      current_database_info_ = v.dbinfo;
      databases_.push_back(current_database_info_);
    }
  }
  emit LoadDiscoveryInfoFinished(v);
}

void IServer::HandleClearServerHistoryResponceEvent(events::ClearServerHistoryResponceEvent* ev) {
  auto v = ev->value();
  common::Error er = v.errorInfo();
  if (er && er->IsError()) {
    LOG_ERROR(er, true);
  }

  emit ClearServerHistoryFinished(v);
}

void IServer::ProcessDiscoveryInfo(const events_info::DiscoveryInfoRequest& req) {
  emit LoadDiscoveryInfoStarted(req);
  QEvent* ev = new events::DiscoveryInfoRequestEvent(this, req);
  Notify(ev);
}

}  // namespace proxy
}  // namespace fastonosql
