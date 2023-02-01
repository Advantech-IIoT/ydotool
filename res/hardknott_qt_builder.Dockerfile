FROM advantechiiot/imx-yocto-sdk:hardknott

WORKDIR /src
ENTRYPOINT ["/sdk_setenv.sh"]