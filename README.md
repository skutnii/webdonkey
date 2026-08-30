# webdonkey

This is an early stage header-only C++ web application framework 
based on Boost::beast. 
The name webdonkey was chosen because donkeys are small but helpful beasts of burden.

Currently implemented:
- dependency injection (contextual.hpp);
- general-purpose coroutines (continueation.hpp, coroutines.hpp);
- basic static HTTP and HTTPS server functionality (tcp_listener.hpp, http.hpp, static_responder.hpp).

Examples serve as tests and illustration.

To run on Fedora

```console
sudo setcap CAP_NET_BIND_SERVICE=+eip _build/Debug/examples/donkey_http
```