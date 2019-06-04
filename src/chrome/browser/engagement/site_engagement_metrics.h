// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENGAGEMENT_SITE_ENGAGEMENT_METRICS_H_
#define CHROME_BROWSER_ENGAGEMENT_SITE_ENGAGEMENT_METRICS_H_

#include <vector>

#include "base/gtest_prod_util.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "url/gurl.h"

namespace mojom {
class SiteEngagementDetails;
}

// Helper class managing the UMA histograms for the Site Engagement Service.
class SiteEngagementMetrics {
 public:
  static void RecordTotalSiteEngagement(double total_engagement);
  static void RecordTotalOriginsEngaged(int total_origins);
  static void RecordMeanEngagement(double mean_engagement);
  static void RecordMedianEngagement(double median_engagement);
  static void RecordEngagementScores(
      const std::vector<mojom::SiteEngagementDetails>& details);
  static void RecordOriginsWithMaxEngagement(int total_origins);
  static void RecordOriginsWithMaxDailyEngagement(int total_origins);
  static void RecordPercentOriginsWithMaxEngagement(double percentage);
  static void RecordEngagement(SiteEngagementService::EngagementType type);
  static void RecordDaysSinceLastShortcutLaunch(int days);
  static void RecordScoreDecayedFrom(double score);
  static void RecordScoreDecayedTo(double score);

 private:
  FRIEND_TEST_ALL_PREFIXES(SiteEngagementServiceTest, CheckHistograms);
  FRIEND_TEST_ALL_PREFIXES(SiteEngagementServiceTest,
                           GetTotalNotificationPoints);
  FRIEND_TEST_ALL_PREFIXES(SiteEngagementServiceTest, LastShortcutLaunch);
  FRIEND_TEST_ALL_PREFIXES(SiteEngagementServiceTest, ScoreDecayHistograms);
  FRIEND_TEST_ALL_PREFIXES(SiteEngagementHelperTest,
                           MixedInputEngagementAccumulation);
  static const char kTotalEngagementHistogram[];
  static const char kTotalOriginsHistogram[];
  static const char kMeanEngagementHistogram[];
  static const char kMedianEngagementHistogram[];
  static const char kEngagementScoreHistogram[];
  static const char kEngagementScoreHistogramIsZero[];
  static const char kOriginsWithMaxEngagementHistogram[];
  static const char kOriginsWithMaxDailyEngagementHistogram[];
  static const char kPercentOriginsWithMaxEngagementHistogram[];
  static const char kEngagementTypeHistogram[];
  static const char kEngagementBucketHistogramBase[];
  static const char kDaysSinceLastShortcutLaunchHistogram[];
  static const char kScoreDecayedFromHistogram[];
  static const char kScoreDecayedToHistogram[];

  static std::vector<std::string> GetEngagementBucketHistogramNames();
};

#endif  // CHROME_BROWSER_ENGAGEMENT_SITE_ENGAGEMENT_METRICS_H_
