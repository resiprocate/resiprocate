#
# $Id: Makefile,v 1.2 2002/09/22 21:34:23 fluffy Exp $
#
FSMS=srv-inv-fsm
OUTDIR=out

PSFILES=$(patsubst %, $(OUTDIR)/%.ps, $(FSMS))
PDFFILES=$(patsubst %, $(OUTDIR)/%.pdf, $(FSMS))
SVGFILES=$(patsubst %, $(OUTDIR)/%.svg, $(FSMS))
PNGFILES=$(patsubst %, $(OUTDIR)/%.png, $(FSMS))

FILES=$(PSFILES) $(PDFFILES) $(SVGFILES) $(PNGFILES)


DOT=dot

.phony:

all: $(FILES)

$(OUTDIR):
	-mkdir $@

$(OUTDIR)/%.pdf: $(OUTDIR)/%.ps Makefile $(OUTDIR)
	ps2pdf13 $< $@

$(OUTDIR)/%.ps: %.dot Makefile $(OUTDIR)
	$(DOT) -Tps -o$@ $<

$(OUTDIR)/%.png: %.dot
	$(DOT) -Tpng -o$@ $<

$(OUTDIR)/%.svg: %.dot Makefile $(OUTDIR)
	$(DOT) -Tsvg -o$@ $<

clean: 
	-/bin/rm -f $(FILES) *.ps

dummy:
	cp  *.pdf  ~/public_html/misc/


test:
	-@echo $(FILES)
