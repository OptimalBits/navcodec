
Node libavcodec bindings
------------------------

navcodec is a module that aims to provide a nodejs wrapper for the excelent libavcodec library. The module does not aim to make a 1 to 1 mapping between libavcodec function and structures and javascript. Instead the aim is to create a new javascript API inspired by the underlying library. The initial priority will be to make the transcoding use case, and later move to other uses cases such as playing, filtering, editing, etc.

