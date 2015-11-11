# Developer Details #

This page provides some internal information that may be of interest for developers.


## The Big Picture ##

Here's an overview of how all the pieces of this project link together.

![![](http://protoc-gen-docbook.googlecode.com/svn/wiki/images/big_picture_small.png)](http://protoc-gen-docbook.googlecode.com/svn/wiki/images/big_picture_big.png)

## From _protoc_ to _protoc-gen-docbook_ ##

**protoc-gen-docbook** is a C++ process-based plugin used by the Protobuf Compiler (_protoc_).
The plugin architecture, which is well defined within _protoc_, allows it to load external process for additional language generation support.

Here is the developer oriented documentation in [plugin.pb.h](https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.compiler.plugin.pb)
```
// A plugin executable needs only to be placed somewhere in the path.  The
// plugin should be named "protoc-gen-$NAME", and will then be used when the
// flag "--${NAME}_out" is passed to _protoc_.
```

Therefore, _protoc-gen-docbook_ relies on entirely on _protoc_ to parse the input .proto source file. The parse results are then encoded in protobuf and be passed into _protoc_-gen-docbook through _stdin_. This information is called the **descriptor**. _protoc_-gen-docbook handles the descriptor information, and generates DocBook output.

If _protoc_-gen-docbook is in the same path as _protoc_, it will be loaded automatically through the following command.

```
protoc --docbook_out=. --proto_path=$your_proto_path $your_proto_file
```

### Protoc 2.5.0 ###

One small but important detail is the Protobuf Compiler version. _protoc-gen-docbook_ can only work with _protoc_ 2.5.0 and above. The reason is because .proto comments are only collected since 2.5.0+. Prior to 2.5.0, all comments are discarded.


---


## Transforming into DocBook ##

The transformation process into DocBook is fairly straight forward. Since .proto are basically composed of files, message, and fields, the structure maps well into DocBook [informaltable](http://oreilly.com/catalog/docbook/book2/informaltable.html)

Here is a sample protobuf message:
```
message Person {

  // This field describes the full name of the person. It should be in
  // lastname/firstname format, and may or may not be unique.
  required string name = 1; 

  // This field describes the unique ID number for this person. 
  required int64 id = 2;

  // This field describes the email address of this person. If this person 
  // does not have a email addres, omit it.
  optional string email = 3;
}
```

The result DocBook will be map to:

```
<informaltable frame="all" xml:id="tutorial_Person">
  <tgroup cols="4">
    <colspec colname="c1" colnum="1" colwidth="3*" />
    <colspec colname="c2" colnum="2" colwidth="2*" />
    <colspec colname="c3" colnum="3" colwidth="2*" />
    <colspec colname="c4" colnum="4" colwidth="6*" />
      <thead>
        <row>
          <?dbhtml bgcolor="#A6B4C4" ?>
          <?dbfo bgcolor="#A6B4C4" ?>
          <entry>Field</entry>
          <entry>Type</entry>
          <entry>Rule</entry>
          <entry>Description</entry>
        </row>
      </thead>      
      <tbody>
        <row>
          <?dbhtml bgcolor="#ffffff" ?>
          <?dbfo bgcolor="#ffffff" ?>
          <entry>name</entry>
          <entry><emphasis role="underline" 
			xlink:href="#protobuf_scalar_value_types">string</emphasis>
		  </entry>
          <entry>required</entry>
          <entry>
            <para> a person&apos;s name, required btw, must be awesome
          </para>
          </entry>
        </row>

        <row>
          <?dbhtml bgcolor="#f0f0f0" ?>
          <?dbfo bgcolor="#f0f0f0" ?>
          <entry>id</entry>
          <entry><emphasis role="underline" 
            link:href="#protobuf_scalar_value_types">int32</emphasis>
		  </entry>
          <entry>required</entry>
          <entry>
            <para>  Unique ID number for this person.</para>
          </entry>
        </row>
        <row>
          <?dbhtml bgcolor="#ffffff" ?>
          <?dbfo bgcolor="#ffffff" ?>
          <entry>email</entry>
          <entry><emphasis role="underline" x
			link:href="#protobuf_scalar_value_types">string</emphasis>
		  </entry>
          <entry>optional</entry>
          <entry></entry>
        </row>
    </tbody>
  </tgroup>
</informaltable>
```


---


## DocBook Into PDF ##

The transformation from DocBook into PDF is done through Apache FOP with DocBook stylesheets. Apache FOP (Apache 2.0 License), and DocBook stylesheets (MIT-like license) are both freely available.

With these powerful tools, transforming DocBook into PDF is simply a command away.

Here's the Windows version of transformation command.
```
cmd /c %fop_path%\fop.bat ^
-xml %docbook_input% ^
-xsl %fop_path%\docbook-xsl-1.78.0\fo\docbook.xsl ^
-pdf %pdf_output% ^
-param page.orientation landscape ^
-param paper.type USletter
```