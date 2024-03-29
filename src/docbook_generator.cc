// protoc-gen-docbook
// http://code.google.com/p/protoc-gen-docbook/
//
// Redistribution and use in source and binary forms, with or without
//// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: askldjd@gmail.com
//
// This file holds the generator that walks through the proto descriptor
// structures and convert them to DocBook tables.
// See http://code.google.com/p/protoc-gen-docbook/ for more information.
//

#include "docbook_generator.h"
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <iomanip>
// For debugging only
//#include <Windows.h>

namespace google { namespace protobuf { namespace compiler { namespace docbook {

namespace utils {

	//! @details
	//! Helper method to trim the beginning and ending white spaces within 
	//! a string.
	string Trim(const string& str)
	{
		string s = str;
		unsigned int p;
		while ((p = s.length()) > 0 && (unsigned char)s[p-1] <= ' ')
			s.resize(p-1);
		while ((p = s.length()) > 0 && (unsigned char)s[0] <= ' ')
			s.erase(0, 1);
		return s;
	}

	//! @details
	//! This method parses a Java like .properties file that is k=v pairs.
	//!
	//! @param[in] filePath
	//! File path to the property file.
	//!
	//! @return
	//! k/v pairs that represents the property file.
	//!
	//! @remark
	//! Hate to reinvent the wheel here since there are about a million 
	//! parser implementations out there. However, I hate to bring in Boost 
	//! library just for something this trivial.
	//!
	//! This parser isn't smart enough to strip out comments on the
	//! same line as the k/v pair.
	std::map <string, string> ParseProperty(string const &filePath)
	{
		std::map <string, string> options;
		std::ifstream ifs(filePath.c_str(), std::ifstream::in);
		if(ifs.is_open())
		{
			string line;
			while(std::getline(ifs, line))
			{
				line = Trim(line);

				// First non-space char that is '#' is a comment line.
				if(line.empty() == false && line[0]=='#') 
					continue;

				int p = 0;
				if ((p = (int)line.find('=')) > 0)
				{
					string k = Trim(line.substr(0, p));
					string v = Trim(line.substr(p+1));
					if (!k.empty() && !v.empty())
						options[k] = v;
				}
			}
		}

		ifs.close();
		return options;
	}
}

namespace {

	//! These options are part of the layout look-and-feel parameters that
	//! adjusts the informaltable column widths.
	char const* OPTION_NAME_FIELD_NAME_COLUMN_WIDTH = "field_name_column_width";
	char const *OPTION_NAME_FIELD_TYPE_COLUMN_WIDTH = "field_type_column_width";
	char const *OPTION_NAME_FIELD_RULE_COLUMN_WIDTH = "field_rules_column_width";
	char const *OPTION_NAME_FIELD_DESC_COLUMN_WIDTH = "field_desc_column_width";

	//! These options are part of the layout look-and-feel parameters that
	//! adjusts the informaltable color scheme
	char const *OPTION_NAME_COLUMN_HEADER_COLOR = "column_header_color";
	char const *OPTION_NAME_ROW_COLOR = "row_color"; // odd rows color
	char const *OPTION_NAME_ROW_COLOR_ALT = "row_color_alt"; // even rows color

	//! @details
	//! Scalar Value Table is a table that holds descriptions for primitive 
	//! types in protobuf (e.g. int32, fixed32, etc). It is a convenient reminder
	//! on what those type means.
	//!
	//! 1 to include 
	//! 0 to exclude
	//!
	char const *OPTION_NAME_INCLUDE_SCALAR_VALUE_TABLE = "include_scalar_value_table";

	//! @details
	//! Each table generated by protoc-gen-docbook is under a <section> tag with
	//! a specific level. (e.g. sec1, sect2 ... sect5)
	//! By adjusting this field, you may increase the first section level used.
	//! [default = 1, must be <= 5]
	char const *OPTION_NAME_STARTING_SECTION_LEVEL = "starting_section_level";

	//! @details
	//! Add a timestamp at the bottom of the document to indicate when it was
	//! generated.
	//!
	//! This option is only available if custom template is NOT used.
	//!
	//! 1 to include
	//! 0 to exclude
	//!
	//! [default = 0]
	char const *OPTION_NAME_INCLUDE_TIMESTAMP = "include_timestamp";

	//! @details
	//! Preserve line breaks within the comment in .proto into the generated DocBook.
	//! This implies all \r\n or \n will be converted into <sbr/>
	//!
	//! 1 to preserve
	//! 0 to ignore (All line breaks in comment converts into space.)
	//!
	//! [default = 0]
	//! See Issue #3
	char const *OPTION_NAME_PRESERVE_COMMENT_LINE_BREAKS = "preserve_comment_line_breaks";

	//! @details
	//! Custom template file allows user to provide their own template file
	//! and use "insertion point" to pinpoint where the tables should be
	//! located. If not provided, a default template is provided.
	char const *OPTION_NAME_CUSTOM_TEMPLATE_FILE = "custom_template_file";

	//! @details
	//! Default output file name, not adjustable at the moment.
	char const *DEFAULT_OUTPUT_NAME = "docbook_out.xml";

	char const *DEFAULT_INSERTION_POINT= "insertion_point";

	char const *SCALAR_TABLE_INSERTION_POINT = "scalar_table";

	char const *SCALAR_VALUE_TYPES_TABLE_XML_ID = "protobuf_scalar_value_types";

	//! @details
	//! Column width measurement
	//!
	//! From DocBook official documentation:
	//! ColWidth specifies the desired width of the relevant column. 
	//! It can be either a fixed measure using one of the CALS units 
	//! (36pt, 10pc, etc.) or a proportional measure. Proportional measures 
	//! have the form �number*�, meaning this column should be number times 
	//! wider than a column with the measure �1*� (or just �*�). These two forms 
	//! can be mixed, as in �3*+1pc�. 
	//! See http://www.docbook.org/tdg/en/html/colspec.html
	char const *DEFAULT_FIELD_NAME_COLUMN_WIDTH = "3";
	char const *DEFAULT_FIELD_TYPE_COLUMN_WIDTH = "2";
	char const *DEFAULT_FIELD_RULES_COLUMN_WIDTH = "2";
	char const *DEFAULT_FIELD_DESC_COLUMN_WIDTH = "6";

	//! @details
	//! The insertion points syntax is defined by protobuf library.
	//! See https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.compiler.plugin.pb
	char const *INSERTION_POINT_START_TAG = "<!-- @@protoc_insertion_point(";
	char const *INSERTION_POINT_END_TAG = ") -->";
	
	//! @details
	//! Highest <section> level, defined by DocBook
	//! see http://oreilly.com/openbook/docbook/book/sect1.html
	int const MAX_SECTION_LEVEL = 5;

	//! @details
	//! Highest <section> level allowed by the user.
	//! protoc-gen-doc needs at least 2 levels to operate correctly
	//! Worst case, we may use sect4 and sect5.
	int const MAX_ALLOWED_SECTION_LEVEL_OPTION = 4;

	//! @details
	//! K/V pair that holds the options name and the options value.
	std::map<string, string> s_docbookOptions;

	//! @details
	//! Static field that controls the row color. May be overridden through
	//! OPTION_NAME_ROW_COLOR.
	string s_rowColor = "ffffff";

	//! @details
	//! Static field that controls the alternate row color. May be 
	//! overridden through OPTION_NAME_ROW_COLOR_ALT.
	string s_rowColorAlt = "f0f0f0";

	//! @details
	//! Static field that controls the header color. May be overridden through
	//! OPTION_NAME_COLUMN_HEADER_COLOR.
	string s_columnHeaderColor = "A6B4C4";

	//! @details
	//! The name of the custom template file.
	//! see s_customTemplateFile
	string s_customTemplateFileName;

	//! @details
	//! Merge the output table into the template file. This is optional.
	//! If template file is not specified, we should run in stand-alone
	//! mode to generate a complete document.
	string s_customTemplateFile;

	//! @details
	//! To include or exclude the scalar value table.
	//! See OPTION_NAME_INCLUDE_SCALAR_VALUE_TABLE
	bool s_includeScalarValueTable = true;

	//! @details
	//! To include or exclude the timestamp in the generated document.
	//! See OPTION_NAME_INCLUDE_TIMESTAMP
	bool s_includeTimestamp = false;

	//! @details
	//! To preserve line breaks from comment field in the generated document.
	//! See OPTION_NAME_PRESERVE_COMMENT_LINE_BREAKS
	bool s_preserve_comment_line_breaks = false;

	//! @details
	//! This field marks the first time DocBookGenerator::Generate method is
	//! called. If it is the first time, we need to generate the template 
	//! DocBook file. 
	//!
	//! @remark
	//! In case you are wondering, yes this is a hack. The problem is that
	//! the DocBookGenerator::Generate by const by contract, and therefore
	//! can only be stateless. This is likely because in normal language, 
	//! every .proto file tends to generate its own source file, but 
	//! DocbookGenerator needs to merge them all into the same xml file. The 
	//! nature of this incompatibility results in this hack.
	bool s_templateFileMade = false;

	//! @details
	//! Name of the DocBook output file.
	string s_docbookOuputFileName = DEFAULT_OUTPUT_NAME;

	//! @details
	//! The starting <sect> used for the generated table.
	int s_startingSectionLevel = 1;

	int const NUM_SCALAR_TABLE_TYPE = 15;
	int const NUM_SCALAR_TABLE_COLUMN = 4;

	//! @details
	//! Giant table used for the Scalar Type Table. This information is
	//! copied directly from protobuf guide.
	char const * s_scalarTable[NUM_SCALAR_TABLE_TYPE][NUM_SCALAR_TABLE_COLUMN] = {
		{
			"double", 
				"", 
				"double", 
				"double"
		} 
		, 
		{
			"float", 
				"", 
				"float", 
				"float"
		} 
		, 	
		{
			"int32", 
				"Uses variable-length encoding. Inefficient for encoding \
				negative numbers - if your field is likely to have negative \
				values, use sint32 instead.", 
				"int32", 
				"int"
		} 
		, 	
		{
			"int64", 
				"Uses variable-length encoding. Inefficient for encoding \
				negative numbers - if your field is likely to have negative \
				values, use sint64 instead.", 
				"int64", 
				"long"
		} 
		, 	
		{
			"uint32", 
				"Uses variable-length encoding.", 
				"uint32", 
				"int"
		} 
		, 	
		{
			"uint64", 
				"	Uses variable-length encoding.", 
				"uint64", 
				"long"
		} 
		, 	
		{
			"sint32", 
				"Uses variable-length encoding. Signed int value. These \
				more efficiently encode negative numbers than regular int32s.", 
				"int32", 
				"int"
		} 
		, 	
		{
			"sint64", 
				"Uses variable-length encoding. Signed int value. These more\
				efficiently encode negative numbers than regular int64s.", 
				"int64", 
				"long"
		} 
		, 	
		{
			"fixed32", 
				"Always four bytes. More efficient than uint32 if values are \
				often greater than 2^28.", 
				"uint32", 
				"int"
		} 
		, 	
		{
			"fixed64", 
				"Always eight bytes. More efficient than uint64 if values \
				are often greater than 2^56.", 
				"uint64", 
				"long"
		} 
		, 	
		{
			"sfixed32", 
				"Always four bytes..", 
				"int32", 
				"int"
		} 
		, 	
		{
			"sfixed64", 
				"Always eight bytes.", 
				"int64", 
				"long"
		} 
		, 	
		{
			"bool", 
				"", 
				"bool", 
				"boolean"
		} 
		, 	
		{
			"string", 
				"	A string must always contain UTF-8 encoded or 7-bit ASCII text.", 
				"string", 
				"String"
		} 
		, 	
		{
			"bytes", 
				"May contain any arbitrary sequence of bytes.", 
				"string", 
				"ByteString"
		} 
	};

	//! @details
	//! Clean up the comment string for any special characters
	//! to ensure it is acceptable in XML format.
	//!
	//! @warning
	//! Currently this method does not handle UTF-8 correctly, and this
	//! should be revisited if unicode comments become a concern.
	//!
	string SanitizeCommentForXML(string const &comment)
	{
		string cleanedComment;
		cleanedComment.reserve(comment.size());
		for(size_t pos = 0; pos != comment.size(); ++pos) 
		{
			char c = comment[pos];
			switch(c) 
			{
			case '&':  
				cleanedComment.append("&amp;");       
				break;
			case '\"': 
				cleanedComment.append("&quot;");      
				break;
			case '\'': 
				cleanedComment.append("&apos;");      
				break;
			case '<':  
				cleanedComment.append("&lt;");
				break;
			case '>':  
				cleanedComment.append("&gt;");
				break;
			case '\n':
				if(s_preserve_comment_line_breaks) 
				{
					cleanedComment.append("<sbr/>");
				}				
				break;
			case '\r':
				cleanedComment.append(" ");				
				break;
			default:
				// Space out all null character because stream can't handle it.
				if(c == 0) 
				{
					c = ' ';
				}
				cleanedComment.append(1, c); 
				break;
			}
		}
		return cleanedComment;
	}


	//! @details
	//! Turn comments into docbook paragraph form by replacing 
	//! every 2 newlines into a paragraph.
	//! Experimental, doesn't seem to work too well in all scenarios.
	string ParagraphFormatComment(string const &comment)
	{
		string cleanComment = SanitizeCommentForXML(comment);

		// Occasionally there are some comments that has nothing but white
		// spaces. This will eliminate those useless fillers and sanitize the
		// comments.
		if(utils::Trim(cleanComment).empty())
		{
			return "";
		}

		size_t index = 0;
		string paragraph  = "<para>";
		paragraph += cleanComment;
		for(;;)
		{
			index = paragraph.find("\n\n", index);
			if (index == string::npos)
				break;

			paragraph.replace(index, 2, "</para>\n<para>");

			index += 3;
		}
		paragraph += "</para>";
		return paragraph;
	}

	//! @details
	//! Helper method that guards the section level field.
	//!
	//! @param[in] sectionLevel
	//! level to guard
	//!
	//! @return
	//! MAX_SECTION_LEVEL if too large, 1 if <= 0
	int SectionLevel(int sectionLevel)
	{
		if(sectionLevel > MAX_SECTION_LEVEL)
			return MAX_SECTION_LEVEL;

		if(sectionLevel <= 0)
			return 1;

		return sectionLevel;
	}

	//! @details
	//! Generate a Xlink for a message. This Xlink allows user to click
	//! and navigate to different messages and enums.
	//!
	//! @param[in,out] string const & messageName
	//! The full globally uniquue message name used to generate the link.
	//!
	//! @param[in,out] string const & displayName
	//! The name of the link to display to user
	//!
	//! @return std::string
	//! The generated Xlink
	//!
	string MakeXLink(string const &messageName, string const &displayName)
	{
		std::ostringstream os;

		// Can't have "." in link, so replace them with underscores.
		string refName = messageName;
		std::replace(refName.begin(), refName.end(), '.', '_');

		os 
			<< "<emphasis role=\"underline\""
			<< " xlink:href=\"" << "#" << refName << "\">"
			<< displayName<< "</emphasis>";

		return os.str();
	}

	//! @details
	//! Generate a Xlink to the Scalar Table. 
	//! See OPTION_NAME_INCLUDE_SCALAR_VALUE_TABLE
	//!
	//! @param[in,out] string const & displayName
	//! The name of the link to display to user
	//!
	//! @return std::string
	//! The generated Xlink
	//!
	string MakeXLinkScalarTable(string const &displayName)
	{
		std::ostringstream os;

		os 
			<< "<emphasis role=\"underline\""
			<< " xlink:href=\"" << "#" << SCALAR_VALUE_TYPES_TABLE_XML_ID << "\">"
			<< displayName<< "</emphasis>";

		return os.str();
	}

	//! @details
	//! Helper method to return an informative string if "packed" option
	//! is enabled.
	//! See https://developers.google.com/protocol-buffers/docs/proto
	string MakePackedString(FieldDescriptor const *fd)
	{
		if(fd->is_packed())
		{
			return "[packed = true]";
		}
		return "";
	}


	//! @details
	//! This method generate an informative default string if the field
	//! has a default value.
	//!
	//! @param[in,out] FieldDescriptor const * fd
	//! The descriptor of the field that may have the default value.
	//!
	//! @return std::string
	//! A string that contains the default value, empty if no defaults.
	//!
	string MakeDefaultValueString(FieldDescriptor const *fd)
	{
		std::ostringstream defaultStringOs;
		if(fd->has_default_value())
		{
			defaultStringOs << "\n[default = ";
			switch(fd->type())
			{
			case FieldDescriptor::TYPE_BOOL:
				{
					if(fd->default_value_bool())
						defaultStringOs << "true";
					else 
						defaultStringOs << "false";
				}
				break;
			case FieldDescriptor::TYPE_BYTES:
				{
					std::string const &bytes = fd->default_value_string();
					std::ostringstream ss;
					ss << std::hex << std::uppercase << std::setfill( '0' );
					for(unsigned int i=0; i<bytes.size(); ++i)
					{
						ss << std::setw( 2 ) << (int)bytes[i]<< ' ';
					}

					defaultStringOs << ss.str();
				}
				break;
			case FieldDescriptor::TYPE_STRING:
				defaultStringOs << SanitizeCommentForXML(fd->default_value_string());
				break;
			case FieldDescriptor::TYPE_DOUBLE:
				defaultStringOs << fd->default_value_double();
				break;
			case FieldDescriptor::TYPE_ENUM:
				defaultStringOs << fd->default_value_enum()->name();
				break;
			case FieldDescriptor::TYPE_FIXED32:
				defaultStringOs << fd->default_value_int32();
				break;
			case FieldDescriptor::TYPE_FIXED64:
				defaultStringOs << fd->default_value_int64();
				break;
			case FieldDescriptor::TYPE_FLOAT:
				defaultStringOs << fd->default_value_float();
				break;
			case FieldDescriptor::TYPE_GROUP:
				break;
			case FieldDescriptor::TYPE_INT32:
				defaultStringOs << fd->default_value_int32();
				break;
			case FieldDescriptor::TYPE_INT64:
				defaultStringOs << fd->default_value_int64();
				break;
			case FieldDescriptor::TYPE_SFIXED32:
				defaultStringOs << fd->default_value_int32();
				break;
			case FieldDescriptor::TYPE_SFIXED64:
				defaultStringOs << fd->default_value_int64();
				break;
			case FieldDescriptor::TYPE_SINT32:
				defaultStringOs << fd->default_value_int32();
				break;
			case FieldDescriptor::TYPE_SINT64:
				defaultStringOs << fd->default_value_int64();
				break;
			case FieldDescriptor::TYPE_UINT32:
				defaultStringOs << fd->default_value_uint32();
				break;
			case FieldDescriptor::TYPE_UINT64:
				defaultStringOs << fd->default_value_uint64();
				break;
			default:
				return "";
			}

			defaultStringOs << " ]";
		}

		string packedString = MakePackedString(fd);

		if(packedString.empty() == false) 
		{
			defaultStringOs << " " << packedString;
		}
		return defaultStringOs.str();
	}

	template <typename DescriptorType>
	static string GetDescriptorComment(const DescriptorType* descriptor) {
		SourceLocation location;
		string comments;
		if (descriptor->GetSourceLocation(&location)) {
			comments = location.leading_comments;
			comments += " ";
			comments += location.trailing_comments;
		}

		return comments;
	}

	//! @details
	//! Docbook header that wraps the document under the Article tag.
	//!
	//! @param[in,out] std::ostringstream & os
	//! The accumulated stream.
	void WriteDocbookHeader(std::ostringstream &os)
	{
		os 
			<< "<?xml version=\"1.0\""
			<< " encoding=\"utf-8\""
			<< " standalone=\"no\"?>"
			<< "<article xmlns=\"http://docbook.org/ns/docbook\""
			<< " xmlns:xlink=\"http://www.w3.org/1999/xlink\""
			<< " version=\"5.0\">" 
			<< std::endl;		
	}

	//! @details
	//! Docbook footer that closes the article tag
	//!
	//! @param[in,out] std::ostringstream & os
	//! The accumulated stream.
	void WriteDocbookFooter(std::ostringstream &os)
	{
		if(s_includeTimestamp)
		{
			// Use "Complete ISO date and time, including offset from UTC."
			// See http://www.sagehill.net/docbookxsl/Datetime.html for 
			// formatting options.
			os << "<para>This document was generated <?dbtimestamp \
				  format=\"c\"?>.</para>" << std::endl;
		}
		os << "</article>" << std::endl;		
	}

	void WriteProtoFileHeader(
		std::ostringstream &os, 
		FileDescriptor const *fd, 
		int sectionLevel)
	{
		os 
			<< "<sect" << SectionLevel(sectionLevel) << ">"
			<< "<title> File: " << fd->name() << "</title>" << std::endl;
	}

	//! @details
	//! Proto file footer that closes the file section scope.
	//!
	//! @param[in,out] std::ostringstream & os
	//! The accumulated stream.
	//!
	//! @param[in] int sectionLevel
	//!
	//! @return void
	
	void WriteProtoFileFooter(std::ostringstream &os, int sectionLevel)
	{
		os << "</sect" << SectionLevel(sectionLevel) << ">" << std::endl;
	}

	//! @details
	//! Write the Informal Table Header for a Message type.
	//! This will define the column header, width and style of the
	//! field table.
	void WriteMessageInformalTableHeader(
		std::ostringstream &os, 
		string const &xmlID, 
		string const &title,
		string const &comment,
		int sectionLevel)
	{
		std::map<string,string>::const_iterator itr;

		string paragraphComment = ParagraphFormatComment(comment);

		os 
			<< "<sect" << SectionLevel(sectionLevel) << ">"
			<< "<title> Message: " << title << "</title>" << std::endl
			<< paragraphComment << std::endl
			<< "<informaltable frame=\"all\""
			<< " xml:id=\"" << xmlID << "\">" << std::endl
			<< "<tgroup cols=\"4\">" << std::endl
			<< " <colspec colname=\"c1\" colnum=\"1\" colwidth=\"";

		itr = s_docbookOptions.find(OPTION_NAME_FIELD_NAME_COLUMN_WIDTH);
		if(itr != s_docbookOptions.end())
			os << itr->second;
		else
			os << DEFAULT_FIELD_NAME_COLUMN_WIDTH;

		os
			<< "*\" />"<< std::endl
			<< "<colspec colname=\"c2\" colnum=\"2\""
			<< " colwidth=\"";

		itr = s_docbookOptions.find(OPTION_NAME_FIELD_TYPE_COLUMN_WIDTH);
		if(itr != s_docbookOptions.end())
			os << itr->second;
		else
			os << DEFAULT_FIELD_TYPE_COLUMN_WIDTH;

		os
			<< "*\" />" << std::endl
			<< "<colspec colname=\"c3\" colnum=\"3\""
			<< " colwidth=\"";

		itr = s_docbookOptions.find(OPTION_NAME_FIELD_RULE_COLUMN_WIDTH);
		if(itr != s_docbookOptions.end())
			os << itr->second;
		else
			os << DEFAULT_FIELD_RULES_COLUMN_WIDTH;

		os
			<< "*\" />" << std::endl
			<< "<colspec colname=\"c4\" colnum=\"4\""
			<< " colwidth=\"";

		itr = s_docbookOptions.find(OPTION_NAME_FIELD_DESC_COLUMN_WIDTH);
		if(itr != s_docbookOptions.end())
			os << itr->second;
		else
			os << DEFAULT_FIELD_DESC_COLUMN_WIDTH;

		os
			<<"*\" />" << std::endl
			<< "<thead>" << std::endl
			<< "<row>" << std::endl
			<< "<?dbhtml bgcolor=\"#";

		os << s_columnHeaderColor;

		os
			<<"\" ?>"<< std::endl
			<< "<?dbfo bgcolor=\"#";

		os << s_columnHeaderColor;
		os
			<<"\" ?>"<< std::endl
			<< "\t<entry>Field</entry>" << std::endl
			<< "\t<entry>Type</entry>"<< std::endl
			<< "\t<entry>Rule</entry>"<< std::endl
			<< "\t<entry>Description</entry>"<< std::endl
			<< "</row>"<< std::endl
			<< "</thead>"<< std::endl
			<< "<tbody>"<< std::endl;
	}

	void WriteEnumInformalTableHeader(
		std::ostringstream &os, 
		string const &xmlID, 
		string const &title,
		string const &description,
		int sectionLevel)
	{
		std::map<string,string>::const_iterator itr;

		os 
			<< "<sect" << SectionLevel(sectionLevel) << ">"
			<< "<title> Enum: " << title << "</title>" << std::endl
			<< "<para>" << description << "</para>" << std::endl
			<< "<informaltable frame=\"all\""
			<< " xml:id=\"" << xmlID << "\">" << std::endl
			<< "<tgroup cols=\"3\">" << std::endl
			<< " <colspec colname=\"c1\" colnum=\"1\" colwidth=\"";

		itr = s_docbookOptions.find(OPTION_NAME_FIELD_NAME_COLUMN_WIDTH);
		if(itr != s_docbookOptions.end())
			os << itr->second;
		else
			os << DEFAULT_FIELD_NAME_COLUMN_WIDTH;

		os
			<< "*\" />"<< std::endl
			<< "<colspec colname=\"c2\" colnum=\"2\""
			<< " colwidth=\"";

		itr = s_docbookOptions.find(OPTION_NAME_FIELD_TYPE_COLUMN_WIDTH);
		if(itr != s_docbookOptions.end())
			os << itr->second;
		else
			os << DEFAULT_FIELD_TYPE_COLUMN_WIDTH;

		os
			<< "*\" />" << std::endl
			<< "<colspec colname=\"c3\" colnum=\"3\""
			<< " colwidth=\"";

		itr = s_docbookOptions.find(OPTION_NAME_FIELD_DESC_COLUMN_WIDTH);
		if(itr != s_docbookOptions.end())
			os << itr->second;
		else
			os << DEFAULT_FIELD_RULES_COLUMN_WIDTH;

		os
			<< "*\" />" << std::endl
			<< "<thead>" << std::endl
			<< "<row>" << std::endl
			<< "<?dbhtml bgcolor=\"#";

		os << s_columnHeaderColor;

		os
			<<"\" ?>"<< std::endl
			<< "<?dbfo bgcolor=\"#";

		os << s_columnHeaderColor;

		os
			<<"\" ?>"<< std::endl
			<< "\t<entry>Element</entry>" << std::endl
			<< "\t<entry>Value</entry>"<< std::endl
			<< "\t<entry>Description</entry>"<< std::endl
			<< "</row>"<< std::endl
			<< "</thead>"<< std::endl
			<< "<tbody>"<< std::endl;
	}

	void WriteInformalTableFooter(std::ostringstream &os)
	{
		os 
			<< "</tbody>"<< std::endl
			<< "</tgroup>"<< std::endl
			<< "</informaltable>"<< std::endl;
	}

	//! @details
	//! Writes out the close section tag.
	//! 
	void WriteCloseSection(std::ostringstream &os, int sectionLevel)
	{
		os << "</sect"<< SectionLevel(sectionLevel) << ">" <<std::endl;
	}

	void WriteInformalTableFooter(std::ostringstream &os, int sectionLevel)
	{
		os 
			<< "</tbody>"<< std::endl
			<< "</tgroup>"<< std::endl
			<< "</informaltable>"<< std::endl
			<< "</sect"<< SectionLevel(sectionLevel) << ">" <<std::endl;
	}

	void WriteMessageInformalTableEntry(
		std::ostringstream &os, 
		string const &fieldname,
		string const &type,
		string const &occurrence,
		string const &defaultString,
		string const &comment,
		bool alternateColor)
	{
		string paragraphComment = ParagraphFormatComment(comment);
		string cellcolor = s_rowColor;
		if(alternateColor)
		{
			cellcolor = s_rowColorAlt;
		}
		os 
			<< "<row>"
			<< "<?dbhtml bgcolor=\"#" << cellcolor <<"\" ?>" << std::endl
			<<"<?dbfo bgcolor=\"#" << cellcolor <<"\" ?>"<< std::endl
			<< "\t<entry>" << fieldname << "</entry>" << std::endl
			<< "\t<entry>" << type << "</entry>" << std::endl
			<< "\t<entry>" << occurrence << "</entry>" << std::endl
			<< "\t<entry>" << paragraphComment;

		if(defaultString.empty() == false)
		{
			if(paragraphComment.empty())
			{
				os << defaultString << std::endl;
			}
			else
			{
				os << "<para>" << defaultString << "</para>" << std::endl;
			}
		}

		os << "</entry>" << std::endl;
		
		os
			<< "</row>"<<std::endl
			<< std::endl;
	}

	void WriteEnumInformalTableEntry(
		std::ostringstream &os, 
		string const &fieldname,
		int enumValue,
		string const &comment,
		bool alternateColor)
	{
		string paragraphComment = ParagraphFormatComment(comment);

		string cellcolor = s_rowColor;
		if(alternateColor)
		{
			cellcolor = s_rowColorAlt;
		}

		os 
			<< "<row>"<<std::endl
			<< "<?dbhtml bgcolor=\"#" << cellcolor <<"\" ?>" << std::endl
			<<"<?dbfo bgcolor=\"#" << cellcolor <<"\" ?>"<< std::endl
			<< "\t<entry>" << fieldname << "</entry>" << std::endl
			<< "\t<entry>" << enumValue << "</entry>" << std::endl
			<< "\t<entry>" << paragraphComment << "</entry>" << std::endl
			<< "</row>"<<std::endl
			<< std::endl;
	}

	void WriteMessageFieldEntries( 
		std::ostringstream &os, 
		Descriptor const *messageDescriptor)
	{
		for (int i = 0; i < messageDescriptor->field_count(); i++) {
			string labelStr;

			FieldDescriptor const *fd = messageDescriptor->field(i);

			switch(fd->label())
			{
			case FieldDescriptor::LABEL_OPTIONAL:
				labelStr = "optional";
				break;
			case FieldDescriptor::LABEL_REPEATED:
				labelStr = "repeated";
				break;
			case FieldDescriptor::LABEL_REQUIRED:
				labelStr = "required";
				break;
			}

			string typeName;

			switch(fd->type())
			{
			case FieldDescriptor::TYPE_MESSAGE:
				typeName = MakeXLink(
					fd->message_type()->full_name(),
					fd->message_type()->name());
				break;
			case FieldDescriptor::TYPE_ENUM:
				typeName = MakeXLink(
					fd->enum_type()->full_name(),
					fd->enum_type()->name());
				break;
			default:

				if(s_includeScalarValueTable)
				{
					typeName = MakeXLinkScalarTable(fd->type_name());
				}
				else
				{
					typeName = fd->type_name();
				}
				
				break;
			}

			bool alternateColor = (i%2 != 0);
			WriteMessageInformalTableEntry(
				os, 
				fd->name(),
				typeName,
				labelStr,
				MakeDefaultValueString(fd),
				GetDescriptorComment(fd),
				alternateColor);
		}
	}

	void WriteEnumFieldEntries(
		std::ostringstream &os, 
		EnumDescriptor const *enumDescriptor)
	{
		for (int i = 0; i < enumDescriptor->value_count(); i++) 
		{
			bool alternateColor = (i%2 != 0);
			WriteEnumInformalTableEntry(
				os, 
				enumDescriptor->value(i)->name(),
				i,
				GetDescriptorComment(enumDescriptor->value(i)),
				alternateColor);
		}
	}

	//! @details
	//! This method writes the Enum table that belongs to a descriptor.
	//!
	//! This method is templatized because Enums may live under file or message
	//! descriptor under the protobuf definition. Since it is the same code,
	//! template saves some time.
	//!
	//! @param[in] DescriptorType const * descriptor
	//! The ptr to the descriptor
	//!
	//! @param[in,out] std::ostringstream & os
	//! The accumulated string
	//!
	//! @param[in] string const & prefix
	//! The prefix holds the scope of the enum. E.g. If the enum E is nested with 
	//! message M, the prefix should be "M.". This way, it shows up as M.E in the 
	//! document.
	//!
	//! @param[in] int section
	//! Section level is used to describe which <secX> tag to use.
	//!
	template <typename DescriptorType>
	void WriteEnumTable( 
		DescriptorType const *descriptor, 
		std::ostringstream &os,
		string const &prefix,
		int section)
	{
		for (int i = 0; i < descriptor->enum_type_count(); i++) 
		{
			EnumDescriptor const *enumDescriptor = descriptor->enum_type(i);

			string xmlID = enumDescriptor->full_name();
			std::replace(xmlID.begin(), xmlID.end(),'.','_');

			string enumName;
			if(prefix.empty() == false)
			{
				enumName = prefix;
				enumName += ".";
				enumName += enumDescriptor->name();
			}
			else
			{
				enumName = enumDescriptor->name();
			}

			WriteEnumInformalTableHeader(
				os,
				xmlID, 
				enumName,
				GetDescriptorComment(enumDescriptor),
				section);

			WriteEnumFieldEntries(os, enumDescriptor);
			WriteInformalTableFooter(os, section);
		}
	}

	//! @details
	//! This method writes a table for a single message. If the message
	//! has no fields, no tables will be written.
	//!
	//! @param[in,out] std::ostringstream & os
	//! The accumulated string
	//!
	//! @param[in] Descriptor const * messageDescriptor
	//! The ptr to the message descriptor
	//!
	//! @param[in] string const & descriptorName
	//! The human readable name of this descriptor 
	//!
	//! @param[in] int sectionLevel
	//! Section level is used to describe which <secX> tag to use.
	//!
	//! @return
	//! true if a message is written, false otherwise. This is useful to
	//! determine if a </sectX> tag is needed.
	//!
	bool WriteMessageTable(std::ostringstream &os, 
		Descriptor const *messageDescriptor, 
		string const &descriptorName,
		int sectionLevel)
	{
		if(messageDescriptor->field_count() > 0)
		{
			// XML ID is an unique ID that is used in XLink. Since "." is not 
			// allowed, some sanitization is needed by replacing "." with "_".
			string xmlID = messageDescriptor->full_name();
			std::replace(xmlID.begin(), xmlID.end(),'.', '_');

			WriteMessageInformalTableHeader(
				os,
				xmlID, 
				descriptorName,
				GetDescriptorComment(messageDescriptor),
				SectionLevel(sectionLevel));

			WriteMessageFieldEntries(os, messageDescriptor);

			WriteInformalTableFooter(os);

			return true;
		}

		return false;
	}

	//! @details
	//! Writes the message and recursively traverse its nested type into 
	//! the stream.
	//!
	//! @param[in,out] std::ostringstream & os
	//! Stream to write to.
	//!
	//! @param[in,out] Descriptor const * messageDescriptor
	//! The message descriptor that we are writing at the moment
	//!
	//! @param[in,out] string const & prefix
	//! The prefix that should be appended to the name.
	//!
	//! @param[in,out] int depth
	//! The depth of the recursion we are in.
	//!
	void WriteMessage(
		std::ostringstream &os, 
		Descriptor const *messageDescriptor, 
		string const &prefix, 
		int depth)
	{
		// Append the prefix to descriptor name so that the name will be
		// scoped descriptively. (E.g. name.child_type1.child_type2)
		string descriptorName;
		if(prefix.empty() == false)
		{
			descriptorName = prefix;
			descriptorName += ".";
			descriptorName += messageDescriptor->name();
		}
		else
		{
			descriptorName = messageDescriptor->name();
		}

		// Print this message with all of its field. This should generate a
		// InformalTable type in DocBook for this message.
		bool messageWritten = 
			WriteMessageTable(os, messageDescriptor, descriptorName, depth);

		// Print the enums nested with this message. Since enum is defined 
		// within the message, its section level should be one below the 
		// parent message, hence +3.
		WriteEnumTable(messageDescriptor, os, descriptorName, depth+1);

		// Base case for the recursive call. If there is no nested type, 
		// then the formatting for this message is done.
		// Otherwise, for each nested type, recursively print its own table.
		// Because of the recursive layout, the deepest layered message will
		// be printed last within its root message.
		if(messageDescriptor->nested_type_count() > 0)
		{
			for(int i=0; i<messageDescriptor->nested_type_count(); ++i)
			{
				WriteMessage(
					os, 
					messageDescriptor->nested_type(i), 
					descriptorName, 
					depth+1);
			}
		}

		// If a message was written, close the corresponding section. This 
		// preserves the section hierarchy. See Issue #5.
		if(messageWritten)
		{
			WriteCloseSection(os, depth);
		}
	}

	//! @details
	//! Scalar Value Table is a table that holds descriptions for primitive 
	//! types in protobuf (e.g. int32, fixed32, etc). It is a convenient reminder
	//! on what those type means.
	//! 
	//! see OPTION_NAME_INCLUDE_SCALAR_VALUE_TABLE
	//!
	//! see https://developers.google.com/protocol-buffers/docs/proto
	void WriteScalarValueTable(std::ostringstream &os)
	{
		os 
			<< "<sect1>"
			<< "<title>Scalar Value Types</title>" << std::endl
			<< "<para> A scalar message field can have one of the following types - \
			   the table shows the type specified in the .proto file, and the \
			   corresponding type in the automatically generated class: </para>" << std::endl
			<< "<informaltable frame=\"all\""
			<< " xml:id=\"" << SCALAR_VALUE_TYPES_TABLE_XML_ID << "\">" << std::endl
			<< "<tgroup cols=\"4\">" << std::endl
			<< " <colspec colname=\"c1\" colnum=\"1\" colwidth=\"2*\"/>" << std::endl
			<< " <colspec colname=\"c2\" colnum=\"2\" colwidth=\"6*\"/>" << std::endl
			<< " <colspec colname=\"c3\" colnum=\"3\" colwidth=\"2*\"/>" << std::endl
			<< " <colspec colname=\"c4\" colnum=\"4\" colwidth=\"2*\"/>" << std::endl
			<< "<thead>" << std::endl
			<< "<row>" << std::endl

			<< "<?dbhtml bgcolor=\"#" <<s_columnHeaderColor << "\" ?>" << std::endl
			<< "<?dbfo bgcolor=\"#" <<s_columnHeaderColor << "\" ?>" << std::endl

			<< "<entry>Type</entry>" << std::endl
			<< "<entry>Notes</entry>" << std::endl
			<< "<entry>C++ Type</entry>" << std::endl
			<< "<entry>Java Type</entry>" << std::endl

			<< "</row>" << std::endl
			<< "</thead>" << std::endl
			
			<< "<tbody>" << std::endl;

		int i=0;
		int j=0;
		for(i=0; i<NUM_SCALAR_TABLE_TYPE; ++i)
		{
			string cellcolor = s_rowColor;
			if(i%2 == 1)
			{
				cellcolor = s_rowColorAlt;
			}
			os
				<< "<row>"
				<< "<?dbhtml bgcolor=\"#" <<cellcolor << "\" ?>" << std::endl
				<< "<?dbfo bgcolor=\"#" <<cellcolor << "\" ?>" << std::endl;
			for(j=0; j<NUM_SCALAR_TABLE_COLUMN; ++j)
			{
				os << "<entry>" << s_scalarTable[i][j] << "</entry>" << std::endl;
			}
			os<< "</row>" << std::endl;
		}

		os
			<< "</tbody>" << std::endl
			<< "</tgroup>" << std::endl
			<< "</informaltable>" << std::endl
			<< "</sect1>"<< std::endl;
	}

	//! @details
	//! Makes the template file for the very first call of 
	//! DocGenerator::Generate method. This is needed because 
	//! GeneratorContext::OpenForInsert requires knowledge of the output file.
	//! Hence, GeneratorContext::Open must be called once before 
	//! GeneratorContext::OpenForInsert would work.
	//!
	//! @param[in,out] context
	//! The generator context used to write the file.
	void MakeTemplateFile(GeneratorContext &context)
	{
		// If there is no custom template filename, it implies that we
		// are going to use the default file template.
		if(s_customTemplateFileName.empty())
		{
			std::ostringstream defaultTemplateOs;
			WriteDocbookHeader(defaultTemplateOs);		
			defaultTemplateOs 
				<< INSERTION_POINT_START_TAG
				<< DEFAULT_INSERTION_POINT
				<< INSERTION_POINT_END_TAG
				<< std::endl;

			if(s_includeScalarValueTable)
			{
				WriteScalarValueTable(defaultTemplateOs);
			}

			WriteDocbookFooter(defaultTemplateOs);

			scoped_ptr<io::ZeroCopyOutputStream> output(
				context.Open(s_docbookOuputFileName));

			io::Printer printer(output.get(), '$');
			printer.PrintRaw(defaultTemplateOs.str().c_str());
		}
		// Otherwise, user has provided a template. We use the user's
		// template as base and clone a new file.
		else
		{
			// This section writes the cloned file.
			{
				scoped_ptr<io::ZeroCopyOutputStream> output(
					context.Open(s_docbookOuputFileName));

				io::Printer printer(output.get(), '$');
				printer.PrintRaw(s_customTemplateFile.c_str());
			}
			// This section copies the scalar table if necessary into
			// the file.
			{
				if(s_includeScalarValueTable)
				{
					std::ostringstream os;
					WriteScalarValueTable(os);

					scoped_ptr<io::ZeroCopyOutputStream> output(
						context.OpenForInsert(
						s_docbookOuputFileName, 
						SCALAR_TABLE_INSERTION_POINT));

					io::Printer printer(output.get(), '$');
					printer.PrintRaw(os.str().c_str());
				}
			}
		}
	}

	//! @details
	//! Get the file content into a string buffer.
	//! 
	//! @remarks
	//! Apparently this is one of the fastest method out there.
	//! http://insanecoding.blogspot.com/2011/11/reading-in-entire-file-at-once-in-c.html
	std::string GetFileContent(char const *filename)
	{
		std::string contents;
		std::ifstream in(filename, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			contents.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&contents[0], contents.size());
			in.close();
			return(contents);
		}

		return contents;
	}

	//! 
	//! @details
	//! This method writes the accumulated data buffer into the 
	//! GeneratorContext. This allows protoc framework to take care of the
	//! file IO.
	//!
	//! @param[in,out] std::ostringstream & os
	//! The accumulated stream so far.
	//!
	//! @param[in,out] GeneratorContext * context
	//! The context that we are writing to.
	//!
	//! @param[in,out] string * error
	//! Error strings that may be passed out.
	//!
	//! @param[in,out] string const & fileName
	//! The filename we are writing to.
	//!
	//! @return bool
	//! true if success, false otherwise.
	//!
	bool WriteToDocBookFile(
		std::ostringstream &os, 
		GeneratorContext *context, 
		string *error, 
		string const &fileName)
	{
		if(s_customTemplateFileName.empty())
		{
			// Everything should be appended below the "insertion point" so that 
			// all information is written to a single docbook file.
			scoped_ptr<io::ZeroCopyOutputStream> output(
				context->OpenForInsert(
				s_docbookOuputFileName, 
				DEFAULT_INSERTION_POINT));

			io::Printer printer(output.get(), '$');
			printer.PrintRaw(os.str().c_str());

			if (printer.failed()) 
			{
				*error = "CodeGenerator detected write error.";
				return false;
			}
		}
		else
		{
			scoped_ptr<io::ZeroCopyOutputStream> output(
				context->OpenForInsert(
					s_docbookOuputFileName, 
					fileName));

			io::Printer printer(output.get(), '$');
			printer.PrintRaw(os.str().c_str());

			if (printer.failed()) 
			{
				*error = "CodeGenerator detected write error.";
				return false;
			}
		}
		return true;
	}
} // end anonymous namespace

//! @details
//! Constructor
//! Upon construction, options from docbook.properties (if available) will 
//! be loaded. Under the protoc plugin framework, only one instance of
//! DocbookGenerator is ever created, so options will only be loaded once.
//!
DocbookGenerator::DocbookGenerator()
{
	//! @remark
	//! Wait a bit to allow human being to attach the debugger to this process.
	//! Not sure if this is the best way to debug this type of interprocess
	//! program, but it works well enough for me. (askldjd)
	//Sleep(10000);

	// Upon construction, read the docbook.properties file once to load up
	// all the user options.
	s_docbookOptions = utils::ParseProperty("docbook.properties");

	std::map<string, string>::const_iterator itr;
	itr = s_docbookOptions.find(OPTION_NAME_ROW_COLOR);
	if(itr != s_docbookOptions.end())
	{
		s_rowColor = itr->second;
	}

	itr = s_docbookOptions.find(OPTION_NAME_ROW_COLOR_ALT);
	if(itr != s_docbookOptions.end())
	{
		s_rowColorAlt = itr->second;
	}

	itr = s_docbookOptions.find(OPTION_NAME_COLUMN_HEADER_COLOR);
	if(itr != s_docbookOptions.end())
	{
		s_columnHeaderColor = itr->second;
	}

	itr = s_docbookOptions.find(OPTION_NAME_INCLUDE_SCALAR_VALUE_TABLE);
	if(itr != s_docbookOptions.end())
	{
		if(itr->second == "0")
		{
			s_includeScalarValueTable = false;
		}
		else
		{
			s_includeScalarValueTable = true;
		}
	}

	// User provides a custom template file.
	itr = s_docbookOptions.find(OPTION_NAME_CUSTOM_TEMPLATE_FILE);
	if(itr != s_docbookOptions.end())
	{
		// Copy the content in memory, and if successful, consider this
		// file valid by saving its name.
		s_customTemplateFile = GetFileContent(itr->second.c_str());

		if(s_customTemplateFile.empty() == false)
		{
			s_customTemplateFileName = itr->second;
			s_docbookOuputFileName = s_customTemplateFileName;
			int lastindex = s_docbookOuputFileName.find_last_of("."); 
			s_docbookOuputFileName.insert(lastindex, "-out");
		}
	}

	itr = s_docbookOptions.find(OPTION_NAME_STARTING_SECTION_LEVEL);
	if(itr != s_docbookOptions.end())
	{
		std::istringstream buffer(itr->second);
		buffer >> s_startingSectionLevel;

		if(s_startingSectionLevel <= 0 || 
			s_startingSectionLevel > MAX_ALLOWED_SECTION_LEVEL_OPTION)
		{
			s_startingSectionLevel = 1;
		}
	}

	itr = s_docbookOptions.find(OPTION_NAME_INCLUDE_TIMESTAMP);
	if(itr != s_docbookOptions.end())
	{
		if(itr->second == "0")
		{
			s_includeTimestamp = false;
		}
		else
		{
			s_includeTimestamp = true;
		}
	}

	itr = s_docbookOptions.find(OPTION_NAME_PRESERVE_COMMENT_LINE_BREAKS);
	if(itr != s_docbookOptions.end())
	{
		if(itr->second == "0")
		{
			s_preserve_comment_line_breaks = false;
		}
		else
		{
			s_preserve_comment_line_breaks = true;
		}
	}
}

DocbookGenerator::~DocbookGenerator() 
{
}

//! @details
//! This is the main entry point for this plugin. For each file parsed by 
//! protoc, it will forward the .proto parsed tree into FileDescriptor
//! format. DocbookGenerator will then traverse the information and generate
//! the equivalent DocBook output.
//!
//! @param [in] file
//! The parsed information in FileDescriptor form.
//!
//! @param [in] context
//! The context object is used to concat DocBook stream into the target file.
//! This is part of the protoc framework.
//!
//! @param [in] error
//! String for any error that has occurred.
//!
bool DocbookGenerator::Generate(
	FileDescriptor const *file,
	string const &/*parameter*/,
	GeneratorContext *context,
	string *error) const 
{
	std::ostringstream os;

	WriteProtoFileHeader(os, file, s_startingSectionLevel);

	// Go through each message defined within the file and write their
	// information out recursively.
	for (int i = 0; i < file->message_type_count(); i++) 
	{
		WriteMessage(os, file->message_type(i), "", s_startingSectionLevel+1);
	}

	// Write out the Enums defined within the scope of the file. These
	// enums are not nested within Messages.
	WriteEnumTable(file, os, "", s_startingSectionLevel+1);

	// Close out the Proto and get ready for the next file.
	WriteProtoFileFooter(os, s_startingSectionLevel);

	if(s_templateFileMade == false) 
	{
		s_templateFileMade = true;
		MakeTemplateFile(*context);
	}

	return WriteToDocBookFile(os, context, error, file->name());
}

}}}}  // end namespace
