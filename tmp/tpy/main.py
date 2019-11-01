
#import first_python
#import CppHeaderParser.examples.readSampleClass

#!/usr/bin/env python
""" Usage: call with <filename> <typename>
"""

import sys
import clang.cindex

def parse_struct(node):
    for c in node.get_children():
        parse_node(c)

def parse_function(node):
    #print("function", node.spelling, node.displayname)
    print("function", node.get_definition())
    for c in node.get_children():
        pass
    pass

def parse_var(node):
    #print("var", node.spelling, node.displayName)
    print("function", node.get_definition())
    pass

def parse_node(node):
    if node.kind.is_declaration:
        #print(node.spelling, node.location.file)
        if node.kind == clang.cindex.CursorKind.NAMESPACE:
            print("namespace", node.displayname, "{")
            parse_struct(node)
            print("}")
        elif node.kind == clang.cindex.CursorKind.STRUCT_DECL:
            print("struct", node.displayname, "{")
            parse_struct(node)
            print("};")
        elif node.kind == clang.cindex.CursorKind.CLASS_DECL:
            print("class", node.displayname, "{")
            parse_struct(node)
            print("};")
        elif node.kind == clang.cindex.CursorKind.CXX_METHOD:
            parse_function(node)
        elif node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
            parse_function(node)
        elif node.kind == clang.cindex.CursorKind.VAR_DECL:
            parse_var(node)
    #for c in node.get_children():
    #    parse_node(c)

def find_typerefs(node, fileName):
    """ Find all references to the type named 'typename'
    """
    #if node.kind == clang.cindex.CursorKind.is_declaration
    #if node.kind.is_definition():
    #if node.is_definition():
        #ref_node = clang.cindex.Cursor_ref(node)
        #ref_node = node
    #if node.kind == clang.cindex.CursorKind.ANNOTATE_ATTR:
    #    print("is_definition", node.spelling, node.location.file, node.kind)
    #if node.location.file and node.location.file.name == fileName:
    #if node.spelling != "":
    #    print("data", node.spelling, node.location.file, node.location.line, node.location.column)
    # Recurse for children of this node
    #for c in node.get_children():
    #    find_typerefs(c, fileName)
    for c in node.get_children():
        if c.location and c.location.file.name == fileName:
            parse_node(c)
        #else:
        #    print(c.kind, c.location)


index = clang.cindex.Index.create()
tu = index.parse("test.cpp")
print ('Translation unit:', tu.spelling)
find_typerefs(tu.cursor, "test.cpp")
