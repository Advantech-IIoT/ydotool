FROM advantechiiot/qcom-yocto-sdk:qli_1.7

WORKDIR /src
ENTRYPOINT ["/sdk_setenv.sh"]