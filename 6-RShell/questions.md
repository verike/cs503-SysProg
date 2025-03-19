1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

_answer here_:
Can handle partial reads by looping until the full message / data is received.


2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

_answer here_:
- Using delimiters such as \n, \0, or connection closure.

3. Describe the general differences between stateful and stateless protocols.

_answer here_ : 
- Stateful protocols : Has more robust capabilities such as reliability, but generally be slower with more overhead.
- Stateless protocols: Is faster, will have less overhead, will not be fully reliable.

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

_answer here_:
It’s faster and useful for live applications like social media apps, video calls, gaming, and streaming, whereever speed matters more than reliability.

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

_answer here_ : Sockets are the programming interface provided by the enabling applications to write network programs.