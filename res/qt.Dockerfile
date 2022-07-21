FROM advantechiiot/imx-yocto-sdk:hardknott

COPY . /src/
WORKDIR /src
ENTRYPOINT ["/sdk_setenv.sh"]