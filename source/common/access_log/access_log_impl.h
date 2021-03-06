#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "envoy/access_log/access_log.h"
#include "envoy/config/filter/accesslog/v2/accesslog.pb.h"
#include "envoy/request_info/request_info.h"
#include "envoy/runtime/runtime.h"
#include "envoy/server/access_log_config.h"

#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace AccessLog {

/**
 * Access log filter factory that reads from proto.
 */
class FilterFactory {
public:
  /**
   * Read a filter definition from proto and instantiate a concrete filter class.
   */
  static FilterPtr fromProto(const envoy::config::filter::accesslog::v2::AccessLogFilter& config,
                             Runtime::Loader& runtime);
};

/**
 * Base implementation of an access log filter that performs comparisons.
 */
class ComparisonFilter : public Filter {
protected:
  ComparisonFilter(const envoy::config::filter::accesslog::v2::ComparisonFilter& config,
                   Runtime::Loader& runtime);

  bool compareAgainstValue(uint64_t lhs);

  envoy::config::filter::accesslog::v2::ComparisonFilter config_;
  Runtime::Loader& runtime_;
};

/**
 * Filter on response status code.
 */
class StatusCodeFilter : public ComparisonFilter {
public:
  StatusCodeFilter(const envoy::config::filter::accesslog::v2::StatusCodeFilter& config,
                   Runtime::Loader& runtime)
      : ComparisonFilter(config.comparison(), runtime) {}

  // AccessLog::Filter
  bool evaluate(const RequestInfo::RequestInfo& info,
                const Http::HeaderMap& request_headers) override;
};

/**
 * Filter on total request/response duration.
 */
class DurationFilter : public ComparisonFilter {
public:
  DurationFilter(const envoy::config::filter::accesslog::v2::DurationFilter& config,
                 Runtime::Loader& runtime)
      : ComparisonFilter(config.comparison(), runtime) {}

  // AccessLog::Filter
  bool evaluate(const RequestInfo::RequestInfo& info,
                const Http::HeaderMap& request_headers) override;
};

/**
 * Base operator filter, compose other filters with operation
 */
class OperatorFilter : public Filter {
public:
  OperatorFilter(const Protobuf::RepeatedPtrField<
                     envoy::config::filter::accesslog::v2::AccessLogFilter>& configs,
                 Runtime::Loader& runtime);

protected:
  std::vector<FilterPtr> filters_;
};

/**
 * *And* operator filter, apply logical *and* operation to all of the sub filters.
 */
class AndFilter : public OperatorFilter {
public:
  AndFilter(const envoy::config::filter::accesslog::v2::AndFilter& config,
            Runtime::Loader& runtime);

  // AccessLog::Filter
  bool evaluate(const RequestInfo::RequestInfo& info,
                const Http::HeaderMap& request_headers) override;
};

/**
 * *Or* operator filter, apply logical *or* operation to all of the sub filters.
 */
class OrFilter : public OperatorFilter {
public:
  OrFilter(const envoy::config::filter::accesslog::v2::OrFilter& config, Runtime::Loader& runtime);

  // AccessLog::Filter
  bool evaluate(const RequestInfo::RequestInfo& info,
                const Http::HeaderMap& request_headers) override;
};

/**
 * Filter out HC requests.
 */
class NotHealthCheckFilter : public Filter {
public:
  NotHealthCheckFilter() {}

  // AccessLog::Filter
  bool evaluate(const RequestInfo::RequestInfo& info,
                const Http::HeaderMap& request_headers) override;
};

/**
 * Filter traceable requests.
 */
class TraceableRequestFilter : public Filter {
public:
  // AccessLog::Filter
  bool evaluate(const RequestInfo::RequestInfo& info,
                const Http::HeaderMap& request_headers) override;
};

/**
 * Filter that uses a runtime feature key to check if the log should be written.
 */
class RuntimeFilter : public Filter {
public:
  RuntimeFilter(const envoy::config::filter::accesslog::v2::RuntimeFilter& config,
                Runtime::Loader& runtime);

  // AccessLog::Filter
  bool evaluate(const RequestInfo::RequestInfo& info,
                const Http::HeaderMap& request_headers) override;

private:
  Runtime::Loader& runtime_;
  const std::string runtime_key_;
};

/**
 * Access log factory that reads the configuration from proto.
 */
class AccessLogFactory {
public:
  /**
   * Read a filter definition from proto and instantiate an Instance.
   */
  static InstanceSharedPtr fromProto(const envoy::config::filter::accesslog::v2::AccessLog& config,
                                     Server::Configuration::FactoryContext& context);
};

/**
 * Access log Instance that writes logs to a file.
 */
class FileAccessLog : public Instance {
public:
  FileAccessLog(const std::string& access_log_path, FilterPtr&& filter, FormatterPtr&& formatter,
                Envoy::AccessLog::AccessLogManager& log_manager);

  // AccessLog::Instance
  void log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
           const RequestInfo::RequestInfo& request_info) override;

private:
  Filesystem::FileSharedPtr log_file_;
  FilterPtr filter_;
  FormatterPtr formatter_;
};

} // namespace AccessLog
} // namespace Envoy
