This section assumes that you've installed the required dependencies, prepared your hardware, and configured the firmware. If you haven't done so yet, please head over to the [Prerequisites](../depdenencies) and [Setup](../setup) sections.

All the build system is based on PlatformIO, which will take care of installing all the dependencies automatically.

## Build on a *NIX host

We have tested this on a macOS and Linux host. Your mileage may vary.

```bash title="Building the firmware image"
cd RFQuack
make clean build
```


## Build via Docker

We have tested this on a macOS host with Docker Desktop for Mac.

```bash title="Building the firmware image via a Docker container"
cd RFQuack
make docker-build-image
make build-in-docker
```
