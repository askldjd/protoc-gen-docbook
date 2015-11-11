# Use Case #

You have an existing technical document in DocBook format, and you would like to inject the generated DocBook tables from _protoc-gen-docbook_ in your document. If this is what you want to do, read on.

# The General Idea #

For this use case, the general idea is to have _protoc-gen-docbook_ loading in your existing DocBook file, and insert the tables at specific locations based on [insertion points](http://code.google.com/p/protobuf/source/browse/trunk/src/google/protobuf/compiler/plugin.proto?r=294)

An insertion point looks like this:
```
<!-- @@protoc_insertion_point($your_file_name) -->
```
where $your\_file\_name is the .proto file name including the file extension.

It is expected to for one insertion point for every .proto that is fed into _protoc-gen-docbook. Othewise, an error will occur._

# Sample Insertion Point #
Here is a sample DocBook file that use insertion points.

```
<?xml version="1.0" encoding="utf-8" standalone="no"?>
<article xmlns="http://docbook.org/ns/docbook" xmlns:xlink="http://www.w3.org/1999/xlink" version="5.0">
  <sect1>
    <title> person.proto title</title>
    <para> a long description of person.proto </para>    
    <!-- @@protoc_insertion_point(person.proto) -->
    <para>
      <inlinemediaobject>
        <imageobject>
          <imagedata width="8in" align="center"
          fileref="img/pic1.png" scale="75" />
        </imageobject>
      </inlinemediaobject>
    </para>    
  </sect1>
  <sect1>
    <title> search_request.proto title</title>
    <para>
      <inlinemediaobject>
        <imageobject>
          <imagedata width="8in" align="center"
          fileref="img/pic2.png" scale="75" />
        </imageobject>
      </inlinemediaobject>
    </para>    
    <para> a long description of search_request.proto </para>    
    <!-- @@protoc_insertion_point(search_request.proto) -->
  </sect1>
  <sect1>
    <title> search_response.proto title</title>    
    <para> a long description of search_response.proto </para>
    <!-- @@protoc_insertion_point(search_response.proto) -->
  </sect1>
  
  <!-- @@protoc_insertion_point(scalar_table) -->
</article>

```

See [sample page](http://code.google.com/p/protoc-gen-docbook/wiki/Samples#Sample_4) for source and output.

# Commands and Options #

To specify your custom template file, you will need to set the **custom\_template\_file** variable in **docbook.properties** file.

```
#############################################################################
# If custom_template_file is set, protoc-gen-docbook will generate 
# the docbook tables in a copy of the template file.
# 
# Under this mode, protoc-gen-docbook will use the insertion points to
# determine to insert the table within the template file.
# 
# The insertion point syntax is the following: 
# <!-- @@protoc_insertion_point($your_proto_file_name) -->
#
#############################################################################
custom_template_file = custom_template.xml
```

If you are using **run.bat** to perform your conversion, you will also need to modify the **docbook\_input** variable in the batch file. The docbook input file name should be **{$your\_template\_file}-out**.

```
set fop_path=..\fop-1.1
set docbook_input=.\custom_template-out.xml
set pdf_output=.\custom_template-out.pdf

cmd /c %fop_path%\fop.bat ^
-xml %docbook_input% ^
-xsl %fop_path%\docbook-xsl-1.78.0\fo\docbook.xsl ^
-pdf %pdf_output% ^
-param page.orientation landscape ^
-param paper.type USletter

```