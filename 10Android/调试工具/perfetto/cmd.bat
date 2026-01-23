adb shell setprop persist.traced.enable 1
python record_android_trace.py -c config.pbtx -o trace_file.perfetto-trace111