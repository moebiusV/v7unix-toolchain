# V7 yacc's values are ints; the 32V grammar uses them as pointers (->xvalue on an int).  On a
# 32-bit build an int holds a pointer, so give each access the struct it always meant.
s/\(yypvt\[[^]]*\]\|yyval\)->\(xtype\|xloc\|xvalue\|yvalue\|xname\)/((struct exp *)\1)->\2/g
s/\(yypvt\[[^]]*\]\|yyval\)->\(type\|value\|name\|index\)/((struct symtab *)\1)->\2/g
