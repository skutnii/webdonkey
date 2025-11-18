# webdonkey
A small library of C++ web server implementations based on Boost::beast

To run on Fedora

```console
sudo setcap CAP_NET_BIND_SERVICE=+eip _build/Debug/examples/donkey_http/donkey_http
```