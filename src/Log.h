#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Log.h — thread-safe file logger for Metro Smash
//  Usage:  Log::info("Player spawned at %.1f,%.1f", x, z);
//          Log::warn("Shader failed: %s", msg);
//          Log::err ("Fatal: %s", reason);
//  Output: log.txt (next to EXE), one timestamped line per call.
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <string>
#include <mutex>

class Log {
public:
    // Call once at startup (Game::init) — opens log.txt for writing
    static void open(const std::string& path = "log.txt") {
        std::lock_guard<std::mutex> lk(mx());
        if (fp()) return;         // already open
        fp() = fopen(path.c_str(), "w");
        if (fp()) {
            time_t t = time(nullptr);
            char buf[64]; strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
            fprintf(fp(), "======================================\n");
            fprintf(fp(), "  Metro Smash — log started %s\n", buf);
            fprintf(fp(), "======================================\n");
            fflush(fp());
        }
    }

    static void close() {
        std::lock_guard<std::mutex> lk(mx());
        if (fp()) { fclose(fp()); fp() = nullptr; }
    }

    static void info(const char* fmt, ...) {
        va_list ap; va_start(ap, fmt);
        write("INFO ", fmt, ap);
        va_end(ap);
    }
    static void warn(const char* fmt, ...) {
        va_list ap; va_start(ap, fmt);
        write("WARN ", fmt, ap);
        va_end(ap);
    }
    static void err(const char* fmt, ...) {
        va_list ap; va_start(ap, fmt);
        write("ERROR", fmt, ap);
        va_end(ap);
    }

private:
    static FILE*& fp() { static FILE* f = nullptr; return f; }
    static std::mutex& mx() { static std::mutex m; return m; }

    static void write(const char* level, const char* fmt, va_list ap) {
        // Timestamp
        time_t t = time(nullptr);
        struct tm* tm_ = localtime(&t);
        char ts[32];
        strftime(ts, sizeof(ts), "%H:%M:%S", tm_);

        // Format the message
        char msg[2048];
        vsnprintf(msg, sizeof(msg), fmt, ap);

        std::lock_guard<std::mutex> lk(mx());
        if (fp()) {
            fprintf(fp(), "[%s] [%s] %s\n", ts, level, msg);
            fflush(fp());
        }
        // Also print to stderr so CI logs see it (suppressed in GUI build)
#ifndef NDEBUG
        fprintf(stderr, "[%s] [%s] %s\n", ts, level, msg);
#endif
    }
};
