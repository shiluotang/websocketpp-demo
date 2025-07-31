#!/usr/bin/env bash

declare readonly COMMON_NAME="PowerGlobe"
declare readonly ORGNIZATION_UNIT="Technology Department"
declare readonly ORGNIZATION="Tuben Info Tech LTD."
declare readonly PROVINCE="Jin'an"
declare readonly STATE="Shanghai"
declare readonly COUNTRY="CN"
declare readonly KEYBITS=${KEYBITS:-2048}
declare readonly KEYPAIR_FILENAME=keypair.pem
declare readonly PRIKEY_FILENAME=prikey.pem
declare readonly PUBKEY_FILENAME=pubkey.pem
declare readonly CERTIFICATE_FILENAME=crt.pem

function generate_key_pair() {
    openssl genrsa -des3 -out ${KEYPAIR_FILENAME} ${KEYBITS}
}

function key_pair_extract_pubkey() {
    openssl rsa -in ${KEYPAIR_FILENAME} -inform PEM -pubout -out ${PUBKEY_FILENAME}
}

function key_pair_extract_prikey() {
    openssl rsa -in ${KEYPAIR_FILENAME} -out ${PRIKEY_FILENAME} -outform PEM
}

function print_key_only() {
    openssl rsa -in ${PRIKEY_FILENAME} -text -noout
} 

function print_crt_only() {
    openssl x509 -in ${CERTIFICATE_FILENAME} -text -nocert
}

function generate_self_signed_key_pair() {
    openssl req \
        -x509 \
        -newkey rsa:${KEYBITS} \
        -keyout ${PRIKEY_FILENAME} \
        -out ${CERTIFICATE_FILENAME} \
        -sha512 \
        -days 3650 \
        -nodes \
        -subj "/C=${COUNTRY}/ST=${STATE}/L=${PROVINCE}/O=${ORGNIZATION}/OU=${ORGNIZATION_UNIT}/CN=${COMMON_NAME}"
}

function main() {
    # generate_key_pair
    # key_pair_extract_pubkey
    # key_pair_extract_prikey
    if generate_self_signed_key_pair >& /dev/null; then
        print_crt_only
    fi
}

main "${@}"
