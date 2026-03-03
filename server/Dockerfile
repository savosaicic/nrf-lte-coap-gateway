# Build stage
FROM alpine:3.19 AS builder

RUN apk add --no-cache \
    build-base \
    libcoap-dev \
    cjson-dev \
    sqlite-dev

WORKDIR /build
COPY . .
RUN make

# Runtime stage
FROM alpine:3.19

RUN apk add --no-cache \
    libcoap \
    cjson \
    sqlite-libs

WORKDIR /app
COPY --from=builder /build/coap-server .

# CoAP default port (UDP)
EXPOSE 5683/udp

ENTRYPOINT ["./coap-server"]
