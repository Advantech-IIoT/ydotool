FROM advantechiiot/qcom-yocto-sdk:qli_1.8

WORKDIR /src
ENTRYPOINT ["/sdk_setenv.sh"]
