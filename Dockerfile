# Use the official Ubuntu 14.04 image as the base
FROM ubuntu:14.04 as builder

# Set the working directory inside the container
WORKDIR /app

COPY . .

RUN ls -la /app/src
RUN apt update -y && apt-get install build-essential zlib1g-dev  -y 
RUN cd /app/src && make install

FROM ubuntu:14.04
WORKDIR /app/area

COPY --from=builder /app/bin /app/bin
COPY --from=builder /app/area /app/area
EXPOSE 9000


CMD ["/app/area/rom", "-f", "/app/area/mud.cfg"]
