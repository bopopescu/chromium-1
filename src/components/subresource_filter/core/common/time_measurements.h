// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides tools for measuring time intervals and reporting them to
// UMA histograms.
// WARNING: *UMA_HISTOGRAM_* macros in this file are not thread-safe.
// See also: "base/metrics/histogram_macros*.h".
//
// TODO(pkalinnikov): Consider moving content of this file to "base/metrics/*"
// after some refactoring. Note that most of the code generated by the macros
// below is not thread-safe.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_TIME_MEASUREMENTS_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_TIME_MEASUREMENTS_

#include "base/metrics/histogram.h"
#include "base/time/time.h"
#include "components/subresource_filter/core/common/scoped_timers.h"

namespace subresource_filter {

// Creates a scoped object that measures its lifetime using base::ThreadTicks,
// and reports the result in milliseconds as a UMA statistic to a histogram with
// the provided |name| which is expected to be a runtime constant. The histogram
// collects times up to 10 seconds in 50 buckets.
//
// Under the hood there is a static base::HistogramBase* pointer initialized
// right before the scoped object. The pointer is used by a specific
// |export_functor| passed in to a SCOPED_THREAD_TIMER (see it above).
//
// Example:
//   void Function() {
//     SCOPED_UMA_HISTOGRAM_THREAD_TIMER("Component.FunctionTime");
//     ... Useful things happen here ...
//   }
//
// WARNING: The generated code is not thread-safe.
#define SCOPED_UMA_HISTOGRAM_THREAD_TIMER(name)                             \
  IMPL_SCOPED_UMA_HISTOGRAM_TIMER_EXPANDER(                                 \
      name, impl::ThreadTicksProvider, impl::ExportMillisecondsToHistogram, \
      10 * 1000, __COUNTER__)

// Similar to SCOPED_UMA_HISTOGRAM_THREAD_TIMER above, but the histogram
// collects times in microseconds, up to 1 second, and using 50 buckets.
//
// WARNING: The generated code is not thread-safe.
#define SCOPED_UMA_HISTOGRAM_MICRO_THREAD_TIMER(name)                       \
  IMPL_SCOPED_UMA_HISTOGRAM_TIMER_EXPANDER(                                 \
      name, impl::ThreadTicksProvider, impl::ExportMicrosecondsToHistogram, \
      1000 * 1000, __COUNTER__)

// Similar to SCOPED_UMA_HISTOGRAM_TIMER in "base/metrics/histogram_macros.h",
// but the histogram stores times in microseconds, up to 1 second, in 50
// buckets.
//
// WARNING: The generated code is not thread-safe.
#define SCOPED_UMA_HISTOGRAM_MICRO_TIMER(name)                            \
  IMPL_SCOPED_UMA_HISTOGRAM_TIMER_EXPANDER(                               \
      name, impl::TimeTicksProvider, impl::ExportMicrosecondsToHistogram, \
      1000 * 1000, __COUNTER__)

// Similar to UMA_HISTOGRAM_TIMES in "base/metrics/histogram_macros.h", but
// the histogram stores times in microseconds, up to 1 second, in 50 buckets.
//
// WARNING: The generated code is not thread-safe.
#define UMA_HISTOGRAM_MICRO_TIMES(name, sample)                          \
  UMA_HISTOGRAM_CUSTOM_MICRO_TIMES(name, sample,                         \
                                   base::TimeDelta::FromMicroseconds(1), \
                                   base::TimeDelta::FromSeconds(1), 50)

// This can be used when the default ranges are not sufficient. This macro lets
// the metric developer customize the min and max of the sampled range, as well
// as the number of buckets recorded.
#define UMA_HISTOGRAM_CUSTOM_MICRO_TIMES(name, sample, min, max, bucket_count) \
  IMPL_UMA_HISTOGRAM_ADD(name, sample.InMicroseconds(), min.InMicroseconds(),  \
                         max.InMicroseconds(), bucket_count)

// -----------------------------------------------------------------------------
// Below are helpers used by other macros. Shouldn't be used directly. ---------

// This is necessary to expand __COUNTER__ to an actual value.
#define IMPL_SCOPED_UMA_HISTOGRAM_TIMER_EXPANDER(               \
    name, time_provider, histogram_exporter, max_value, suffix) \
  IMPL_SCOPED_UMA_HISTOGRAM_TIMER_UNIQUE(                       \
      name, time_provider, histogram_exporter, max_value, suffix)

// Creates a static histogram pointer and a scoped object referring to it
// throught the |histogram_exporter| functor. Both the pointer and the scoped
// object are uniquely-named, using the unique |suffix| passed in.
#define IMPL_SCOPED_UMA_HISTOGRAM_TIMER_UNIQUE(                            \
    name, time_provider, histogram_exporter, max_value, suffix)            \
  IMPL_DEFINE_STATIC_UMA_HISTOGRAM_POINTER(name, 1, max_value, 50, suffix) \
  auto scoped_uma_histogram_timer_##suffix =                               \
      impl::ScopedTimerImplFactory<time_provider>::Start(                  \
          histogram_exporter(histogram_##suffix));

// This is necessary to expand __COUNTER__ to an actual value.
#define IMPL_UMA_HISTOGRAM_MICRO_TIMES_EXPANDER(name, max_value, suffix, \
                                                sample)                  \
  IMPL_UMA_HISTOGRAM_MICRO_TIMES_UNIQUE(name, max_value, suffix, sample)

// Defines a static UMA histogram pointer and writes a |sample| to it.
#define IMPL_UMA_HISTOGRAM_ADD(name, sample, min, max, bucket_count)          \
  do {                                                                        \
    IMPL_DEFINE_STATIC_UMA_HISTOGRAM_POINTER(name, min, max, bucket_count, 0) \
    histogram_0->Add(sample);                                                 \
  } while (0)

// Defines a static pointer to a UMA histogram.
//
// WARNING: Static local variable initialization is deliberately *not*
// thread-safe in Chrome builds. See the "-fno-threadsafe-statics" flag in
// "build/config/compiler/BUILD.gn" and "/Zc:threadSafeInit-" in
// "build/config/win/BUILD.gn" for details.
#define IMPL_DEFINE_STATIC_UMA_HISTOGRAM_POINTER(name, min, max, bucket_count, \
                                                 suffix)                       \
  static base::HistogramBase* histogram_##suffix =                             \
      base::Histogram::FactoryGet(                                             \
          name, min, max, bucket_count,                                        \
          base::HistogramBase::kUmaTargetedHistogramFlag);

namespace impl {

// ExportFunctor implementation that puts measurements into a UMA |histogram|.
template <bool is_microsec_precision>
class ExportTimeDeltaToHistogram {
 public:
  ExportTimeDeltaToHistogram(base::HistogramBase* histogram)
      : histogram_(histogram) {}

  void operator()(base::TimeDelta duration) {
    if (is_microsec_precision)
      histogram_->Add(duration.InMicroseconds());
    else
      histogram_->Add(duration.InMilliseconds());
  }

 private:
  base::HistogramBase* histogram_;
};

using ExportMillisecondsToHistogram = ExportTimeDeltaToHistogram<false>;
using ExportMicrosecondsToHistogram = ExportTimeDeltaToHistogram<true>;

}  // namespace impl

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_TIME_MEASUREMENTS_
