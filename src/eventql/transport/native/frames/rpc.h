/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <string>
#include <vector>
#include "eventql/eventql.h"
#include "eventql/util/return_code.h"
#include "eventql/util/util/binarymessagewriter.h"
#include "eventql/transport/native/native_connection.h"
#include "eventql/sql/svalue.h"

namespace eventql {
namespace native_transport {

class RPCFrame {
public:

  RPCFrame();

  void setMethod(const std::string& method);
  void setContentType(const std::string& method);
  void setDatabase(const std::string& method);
  void setBody(const std::string& body);

  void writeToString(std::string* str, bool header = true);
  void clear();

protected:
  uint64_t flags_;
  std::string method_;
  std::string content_type_;
  std::string database_;
  std::string body_;
};

} // namespace native_transport
} // namespace eventql
