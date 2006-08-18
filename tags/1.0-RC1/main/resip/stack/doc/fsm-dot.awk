# For making the Preparse FSM diagram
BEGIN {
  if ( output == "Preparse.ps" )
  {
    doSpecial = 1;
  }
  printf "digraph pp_fsm {\nsize=\"10,8\"\n";
  if ( doSpecial ) 
    {
      printf "rotate=90\n";
      printf "ratio=0.8\n";
    }
  printf "compound=true\nfontsize=18\nfontname=\"Helvetica\"\n";
  printf "node [ fontname=\"Helvetica\" ]\n";
  printf "graph	[\nfontsize=8\nfontname=\"Helvetica\"\nlabelfontsize=8\n";
  printf "labelfontname=\"Helvetica\"\n]\n";
  printf "edge\n[\nfontname=\"Helvetica\"\nfontsize=8\n	arrowhead=normal\n]\n";
}

/^[ ]*AE\(/ {
  line = gensub("\/\/.*$","",g);
  line = gensub("^[ ]*AE\\(","","g",line);
  line = gensub("[\);]","","g",line);
  line = gensub("act","","g",line);

  split(line,f,",");

  if (f[3] == "XC") f[3] = "*";

  if (f[2] != "X")
    printf "%s -> %s [ label=\"%s, %s (%s)\"]\n", f[1], f[4], f[3], f[5], f[2];
  else
    printf "%s -> %s [ label=\"%s, %s\"]\n", f[1], f[4], f[3], f[5];
}
END {
  print "}";
};
