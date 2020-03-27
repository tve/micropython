# test that modtls produces a text error message

try:
    import usocket as socket, ussl as ssl, sys
except:
    import socket, ssl, sys


def test(addr):
    s = socket.socket()
    s.connect(addr)
    try:
        s = ssl.wrap_socket(s)
        print("wrap: True")
    except OSError as e:
        # mbedtls produces "mbedtls -0x7200: SSL - An invalid SSL record was received"
        # axtls produces "RECORD_OVERFLOW"
        # print(e)
        print("wrap:", "invalid SSL record" in str(e) or "RECORD_OVERFLOW" in str(e))
    s.close()


if __name__ == "__main__":
    # connect to plain HTTP port, oops!
    addr = socket.getaddrinfo("micropython.org", 80)[0][-1]
    test(addr)
