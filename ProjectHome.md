# What is it? #
**protoc-gen-docbook** is a [Protocol Buffers](http://code.google.com/p/protobuf/) [plugin](https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.compiler.plugin.pb) for [DocBook](http://www.docbook.org/).

This project aims to support generating documentation in DocBook format with .proto source files. The DocBook format can then be converted to professional looking PDF through [Apache FOP](http://xmlgraphics.apache.org/fop/).

# Why? #
Although protobuf is an excellent data interchange format, it comes with very little documentation support. C-style comments within .proto, while useful for developers, does not scale well for large interfaces. Good-looking documentation is needed to enhance .proto readability and ease the pain of inter-file navigation.

# Getting Started #
See [Quick Start Guide](QuickStartGuide.md) for more information.

# Features #
  * Generate _good_ _looking_ tables through your .proto source file.
  * Output DocBook format, and convertable to .pdf through Apache FOP.
  * Comments in .proto are preserved into document.
  * Field are interlinked for fast file navigation.
  * Customizable look-and-feel on cell color, table width, and more.
  * [Merge table ](CustomTemplateGuide.md) directly in your existing DocBook file.

# Screenshots #

See [Samples](http://code.google.com/p/protoc-gen-docbook/wiki/Samples) page for more sample output.

| **From .proto** | **To DocBook PDF** |
|:----------------|:-------------------|
| [![](http://protoc-gen-docbook.googlecode.com/svn/wiki/images/demo_ugly_sm.png)](http://protoc-gen-docbook.googlecode.com/svn/wiki/images/demo_ugly.PNG) | [http://protoc-gen-docbook.googlecode.com/svn/wiki/images/demo\_nice\_sm.PNG](http://protoc-gen-docbook.googlecode.com/svn/wiki/images/demo_nice.PNG) |

# Update #
**2013-08-23: Version 0.3.1**

  * Resolved [Issue#3](https://code.google.com/p/protoc-gen-docbook/issues/detail?id=#3), [Issue#4](https://code.google.com/p/protoc-gen-docbook/issues/detail?id=#4), and [Issue#5](https://code.google.com/p/protoc-gen-docbook/issues/detail?id=#5).

**2013-03-19: Version 0.3.0**

  * Updated the plugin to the protobuf 2.5.0 baseline (from rc1).

**2013-02-15: Version 0.2.1**

  * Added deployment framework
  * Minor bug fixes
  * Added linux build framework

**2013-02-13: Version 0.2.0**

  * Added support for custom template DocBook file.
  * Enhanced support for various protobuf tags. (e.g. default, packed, etc).
  * Various bug fixes.

# How to contribute? #
You are fully encouraged to download/clone/hack the source code to your fullest extent, and share your results with [everyone](http://code.google.com/p/protoc-gen-docbook/issues/list).