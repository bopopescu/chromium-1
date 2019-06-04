#!/bin/sh

files_giving_yocto_warning="
      ./src/third_party/perfetto/test/data
      ./src/third_party/perfetto/ui/src/gen
      ./src/third_party/skia/tools/gyp
      ./src/buildtools/third_party/libc++/trunk/test/std/input.output/filesystems/Inputs/static_test_env/bad_symlink"

all_git_dirs=`find . -type d -name .git`
all_cipd_dirs=`find . -type d -name .cipd*`

files_over_github_size_limit=`find . -size +99M`

deprecated_nonessential_dirs="
      ./src/third_party/WebKit/LayoutTests/
      ./src/third_party/android_async_task/
      ./src/third_party/apple_sample_code
      ./src/third_party/javax_inject
      ./src/tools/gyp"

nonessential_dirs_removed_earlier="
      ./src/android_webview/tools/system_webview_shell/layout_tests/
      ./src/build/fuchsia/layout_test_proxy/
      ./src/chrome/test/data/extensions/perf_tests/
      ./src/chrome/test/data/printing/layout_tests/
      ./src/ios/
      ./src/native_client*
      ./src/third_party/blink/manual_tests
      ./src/third_party/blink/perf_tests
      ./src/third_party/angle/src/tests/perf_tests
      ./src/third_party/catapult/telemetry/third_party/WebKit/PerformanceTests
      ./src/third_party/openh264/src/autotest/performanceTest
      ./src/third_party/hunspell_dictionaries
      ./src/third_party/android_data_chart/
      ./src/third_party/android_deps/
      ./src/third_party/android_media/
      ./src/third_party/android_opengl/
      ./src/third_party/android_platform/
      ./src/third_party/android_protobuf/
      ./src/third_party/android_sdk/
      ./src/third_party/android_support_test_runner/
      ./src/third_party/android_swipe_refresh/
      ./src/third_party/android_system_sdk/
      ./src/third_party/apk-patch-size-estimator
      ./src/third_party/arcore-android-sdk
      ./src/third_party/fuchsia-sdk
      ./src/third_party/gvr-android-keyboard
      ./src/third_party/gvr-android-sdk
      ./src/third_party/win_build_output
      ./src/android_webview
      ./src/chromecast
      ./src/build/linux/debian_sid_*
      ./src/webrunner
      ./src/tools/android/
      ./src/tools/win/
      ./src/tools/android/
      ./src/third_party/webgl/src/conformance-suites
      ./src/third_party/webgl/src/sdk/tests"

fuchsia_paths=`find . -path '*fuchsia*' -type d`

essential_files="
      ./src/chromecast/common/mojom/
      ./src/third_party/apple_apsl/
      ./src/chrome/test/data/webui/i18n_process_css_test.html
      ./src/chrome/test/data/webui/web_ui_test.mojom
      ./src/v8/test/torque/test-torque.tq
      ./src/v8/test/BUILD.gn
      ./src/chrome/test/data/BUILD.gn
      ./src/v8/test/cctest/BUILD.gn
      ./src/chrome/test/data/webui/BUILD.gn
      ./src/chrome/test/data/media/engagement/preload/BUILD.gn
      ./src/v8/test
      ./src/content/test/data/lite_js_test.mojom
      ./src/content/test/data/mojo_layouttest_test.mojom
      ./src/tools/perf/clear_system_cache/
      ./src/tools/perf/chrome_telemetry_build/
      ./src/tools/perf/BUILD.gn
      ./src/tools/traffic_annotation/auditor/
      ./src/components/test/BUILD.gn
      "

new_nonessential_dirs="
    ./src/chrome/common/extensions/docs
    ./src/third_party/blink/web_tests/
    ./src/third_party/hunspell/tests
    ./src/third_party/liblouis/src/tests/harness
    ./src/third_party/sqlite/src/test/
    ./src/third_party/swiftshader/third_party/llvm-7.0/llvm/test/
    ./src/third_party/xdg-utils/tests
    ./src/third_party/yasm/source/patched-yasm/modules/arch/x86/tests
    ./src/third_party/yasm/source/patched-yasm/modules/dbgfmts/dwarf2/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/bin/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/coff/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/elf/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/macho/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/rdf/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/win32/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/win64/tests
    ./src/third_party/yasm/source/patched-yasm/modules/objfmts/xdf/tests
    ./src/third_party/angle/third_party/deqp/src/external/vulkancts/mustpass
    ./src/third_party/angle/third_party/deqp/src/android
    ./src/third_party/catapult/third_party/vinn/third_party/v8/mac
    ./src/third_party/catapult/third_party/vinn/third_party/v8/win
    ./src/third_party/angle/third_party/deqp
    ./src/third_party/sqlite/sqlite-src-3250200/test
    ./src/third_party/sqlite/sqlite-src-3250300/test
    ./src/tools/perf
    ./src/tools/traffic_annotation
    ./src/components/test
    ./src/components/policy/android
    ./src/third_party/chromite/
    "
nonessential_test_dirs=" 
    ./src/chrome/test/data
    ./src/content/test/data
    ./src/courgette/testdata
    ./src/extensions/test/data
    ./src/media/test/data
    ./src/third_party/breakpad/breakpad/src/processor/testdata
    ./src/third_party/catapult/tracing/test_data
"

new_paths="
"

#rm -Rf $new_paths
rm -Rf $all_cipd_dirs $files_giving_yocto_warning $all_git_dirs $files_over_github_size_limit $nonessential_dirs_removed_earlier $new_nonessential_dirs $nonessential_test_dirs $fuchsia_paths
#$1 $files_over_github_size_limit
#-rw-rw-r-- 1 lokesh lokesh 210M Jun  3 12:30 ./src/third_party/instrumented_libraries/binaries/msan-chained-origins-trusty.tgz
#-rw-rw-r-- 1 lokesh lokesh 188M Jun  3 12:30 ./src/third_party/instrumented_libraries/binaries/msan-no-origins-trusty.tgz
