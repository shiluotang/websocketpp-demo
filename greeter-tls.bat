@echo off
if not exist prikey.pem call gen-pem.bat
websocketd --port=9000 --ssl --sslkey=prikey.pem --sslcert=crt.pem cscript /nologo greeter.vbs
