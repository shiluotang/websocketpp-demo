@echo off
set COMMON_NAME=some task
set ORGNIZATION_UNIT=some department
set ORGNIZATION=some company
set PROVINCE=some provide
set STATE=some state
set COUNTRY=AB
set KEYBITS=2048
set KEYPAIR_FILENAME=keypair.pem
set PRIKEY_FILENAME=prikey.pem
set PUBKEY_FILENAME=pubkey.pem
set CERTIFICATE_FILENAME=crt.pem

:main
    call :generate_self_signed_key_pair
    exit /b 0

:generate_self_signed_key_pair
    openssl req ^
        -x509 ^
        -newkey rsa:%KEYBITS% ^
        -keyout %PRIKEY_FILENAME% ^
        -out %CERTIFICATE_FILENAME% ^
        -sha512 ^
        -days 3650 ^
        -nodes ^
        -subj "/C=%COUNTRY%/ST=%STATE%/L=%PROVINCE%/O=%ORGNIZATION%/OU=%ORGNIZATION_UNIT%/CN=%COMMON_NAME%"
    goto :return

:return
