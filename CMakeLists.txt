cmake_minimum_required(VERSION 3.20)
project(LIBUserActivityMonitor)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加可执行文件
add_executable(LIBUserActivityMonitor main.cpp
        usr/UserSecurityHandler.cpp
        usr/UserSecurityHandler.h
        usr/SuspiciousLoginAttemptsHandler.cpp
        usr/SuspiciousLoginAttemptsHandler.h
        smtp/MailService.cpp
        smtp/MailService.h
        smtp/SMTPConfiguration.h
        smtp/MailStringAssistant.cpp
        smtp/MailStringAssistant.h
        usr/ip_info/GeoIpLookup.cpp
        usr/ip_info/GeoIpLookup.h
        Generic_Configuration.h
        usr/executer/UserInfoExecuter.cpp
        usr/executer/UserInfoExecuter.h
        usr/utils/WindowsLogHelper.cpp
        usr/utils/WindowsLogHelper.h
        usr/aio_UserActivityHandler.cpp
        usr/aio_UserActivityHandler.h
        usr/log/LogToFile.cpp
        usr/log/LogToFile.h
        runtime/windows/NetFirewallPolicyExecuter.cpp
        runtime/windows/NetFirewallPolicyExecuter.h
        usr/UserExpireHandler.cpp
        usr/UserExpireHandler.h
        runtime/windows/AdminRuntimeChecker.cpp
        runtime/windows/AdminRuntimeChecker.h
        usr/time/Time.cpp
        usr/time/Time.h
        usr/log/LogFilename.h
        usr/Configuration.h
        ProductInfo.h
        runtime/windows/file/FileExecuter.cpp
        runtime/windows/file/FileExecuter.h
        runtime/windows/file/Configuration.h
        runtime/windows/WindowsScheduledTaskHandler.cpp
        runtime/windows/WindowsScheduledTaskHandler.h
        Main.rc
)

add_executable(UserActivator Main_ActivateAllUserOnly.cpp
        usr/UserExpireHandler.cpp
        usr/UserExpireHandler.h
        runtime/windows/AdminRuntimeChecker.cpp
        runtime/windows/AdminRuntimeChecker.h
        usr/log/LogToFile.cpp
        usr/log/LogToFile.h
        usr/time/Time.cpp
        usr/time/Time.h
        usr/log/LogFilename.h
        usr/utils/WindowsLogHelper.cpp
        usr/utils/WindowsLogHelper.h
        ProductInfo.h
        ActivateAllUserOnly.rc
)

add_executable(ArchivElevator tools/ArchivElevator.cpp
        Elevator.rc
)

target_link_options(LIBUserActivityMonitor PRIVATE -static -static-libgcc -static-libstdc++)
target_link_options(UserActivator PRIVATE -static -static-libgcc -static-libstdc++)
target_link_options(ArchivElevator PRIVATE -static -static-libgcc -static-libstdc++)

set(CURL_INC "${PROJECT_SOURCE_DIR}/third_party/curl/include")
set(CURL_FULL_LIB "${PROJECT_SOURCE_DIR}/third_party/curl/lib/libcurl.lib")

target_include_directories(LIBUserActivityMonitor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${CURL_INC}>
)

target_include_directories(UserActivator PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${CURL_INC}>
)

set(SIGNTOOL_PATH "E:/certs/signtool.exe")

add_custom_command(TARGET LIBUserActivityMonitor POST_BUILD
        COMMAND "${SIGNTOOL_PATH}" sign /v /fd sha256
        /f "${CMAKE_SOURCE_DIR}/cert/archiv-security.pfx" 
        /p "????????????"                                 
        /tr http://timestamp.sectigo.com /td sha256
        $<TARGET_FILE:LIBUserActivityMonitor>
        COMMENT "Signing LIBUserActivityMonitor.exe..."
)

add_custom_command(TARGET UserActivator POST_BUILD
        COMMAND "${SIGNTOOL_PATH}" sign /v /fd sha256
        /f "${CMAKE_SOURCE_DIR}/cert/archiv-security.pfx"
        /p "????????????"                                 
        /tr http://timestamp.sectigo.com /td sha256
        $<TARGET_FILE:UserActivator>
        COMMENT "Signing UserActivator.exe..."
)

add_custom_command(TARGET ArchivElevator POST_BUILD
        COMMAND "${SIGNTOOL_PATH}" sign /v /fd sha256
        /f "${CMAKE_SOURCE_DIR}/cert/archiv-security.pfx" 
        /p "????????????"                                 
        /tr http://timestamp.sectigo.com /td sha256
        $<TARGET_FILE:ArchivElevator>
        COMMENT "Signing ArchivElevator.exe..."
)

# 编译后把模板文件夹复制到输出目录
file(COPY ${CMAKE_SOURCE_DIR}/smtp/templates
        DESTINATION ${CMAKE_BINARY_DIR}/smtp/)

file(COPY ${CMAKE_SOURCE_DIR}/third_party/curl/bin/libcurl.dll
        DESTINATION ${CMAKE_BINARY_DIR})

# 注意：静态链接不需要复制 bin 目录下的 dll 了，已移除相关代码

# --- 修改点 2: 链接库与依赖 ---
target_link_libraries(LIBUserActivityMonitor PRIVATE
        ${CURL_FULL_LIB}

        # Windows 业务依赖 (保留你原有的)
        netapi32
        wevtapi
        ole32
        oleaut32
        winhttp
        crypt32
        ws2_32
        advapi32
        kernel32
        user32
        shell32

        ole32
        oleaut32
        uuid

        taskschd

        # MinGW 静态链接 curl 必须的额外系统库 (防止 undefined reference)
        Normaliz
        wldap32
        bcrypt
        z          # zlib 压缩库
)

target_link_libraries( UserActivator PRIVATE
        ${CURL_FULL_LIB}

        netapi32
        wevtapi
        ole32
        oleaut32
        winhttp
        crypt32
        ws2_32
        advapi32
        kernel32
        user32
        shell32

        Normaliz
        wldap32
        bcrypt
        z
)

target_link_libraries(ArchivElevator PRIVATE shell32 Shlwapi)

# --- 修改点 3: 关键宏定义 ---
target_compile_definitions(LIBUserActivityMonitor PRIVATE
        _WIN32_WINNT=0x0A00
        UNICODE
        _UNICODE
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS

        CURL_STATICLIB # 这一行至关重要：告诉编译器使用静态模式
)

target_compile_definitions(UserActivator PRIVATE
        _WIN32_WINNT=0x0A00
        UNICODE
        _UNICODE
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS

        CURL_STATICLIB # 这一行至关重要：告诉编译器使用静态模式
)
