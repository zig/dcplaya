all: mkvmuheader mktgabox mkromimg2

mkvmuheader : mkvmuheader.cxx
	$(CXX) -o $@ $<
mktgabox : mktgabox.cxx
	$(CXX) -o $@ $<
mkromimg2 : mkromimg2.c
	$(CC) -o $@ $< -lz
clean:
	@rm -f mkvmuheader mktgabox mkromimg2

distclean: clean
	
maintainer-clean: distclean
	@rm -f *~

