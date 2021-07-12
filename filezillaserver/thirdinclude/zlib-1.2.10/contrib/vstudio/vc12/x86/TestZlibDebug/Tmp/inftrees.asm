; Listing generated by Microsoft (R) Optimizing Compiler Version 18.00.21005.1 

	TITLE	F:\mycode\Hack12306\zlib-1.2.10\inftrees.c
	.686P
	.XMM
	include listing.inc
	.model	flat

INCLUDELIB MSVCRTD
INCLUDELIB OLDNAMES

PUBLIC	_inflate_copyright
CONST	SEGMENT
_inflate_copyright DB ' inflate 1.2.10 Copyright 1995-2017 Mark Adler ', 00H
?lbase@?1??inflate_table@@9@9 DW 03H			; `inflate_table'::`2'::lbase
	DW	04H
	DW	05H
	DW	06H
	DW	07H
	DW	08H
	DW	09H
	DW	0aH
	DW	0bH
	DW	0dH
	DW	0fH
	DW	011H
	DW	013H
	DW	017H
	DW	01bH
	DW	01fH
	DW	023H
	DW	02bH
	DW	033H
	DW	03bH
	DW	043H
	DW	053H
	DW	063H
	DW	073H
	DW	083H
	DW	0a3H
	DW	0c3H
	DW	0e3H
	DW	0102H
	DW	00H
	DW	00H
	ORG $+2
?lext@?1??inflate_table@@9@9 DW 010H			; `inflate_table'::`2'::lext
	DW	010H
	DW	010H
	DW	010H
	DW	010H
	DW	010H
	DW	010H
	DW	010H
	DW	011H
	DW	011H
	DW	011H
	DW	011H
	DW	012H
	DW	012H
	DW	012H
	DW	012H
	DW	013H
	DW	013H
	DW	013H
	DW	013H
	DW	014H
	DW	014H
	DW	014H
	DW	014H
	DW	015H
	DW	015H
	DW	015H
	DW	015H
	DW	010H
	DW	0c0H
	DW	0caH
	ORG $+2
?dbase@?1??inflate_table@@9@9 DW 01H			; `inflate_table'::`2'::dbase
	DW	02H
	DW	03H
	DW	04H
	DW	05H
	DW	07H
	DW	09H
	DW	0dH
	DW	011H
	DW	019H
	DW	021H
	DW	031H
	DW	041H
	DW	061H
	DW	081H
	DW	0c1H
	DW	0101H
	DW	0181H
	DW	0201H
	DW	0301H
	DW	0401H
	DW	0601H
	DW	0801H
	DW	0c01H
	DW	01001H
	DW	01801H
	DW	02001H
	DW	03001H
	DW	04001H
	DW	06001H
	DW	00H
	DW	00H
?dext@?1??inflate_table@@9@9 DW 010H			; `inflate_table'::`2'::dext
	DW	010H
	DW	010H
	DW	010H
	DW	011H
	DW	011H
	DW	012H
	DW	012H
	DW	013H
	DW	013H
	DW	014H
	DW	014H
	DW	015H
	DW	015H
	DW	016H
	DW	016H
	DW	017H
	DW	017H
	DW	018H
	DW	018H
	DW	019H
	DW	019H
	DW	01aH
	DW	01aH
	DW	01bH
	DW	01bH
	DW	01cH
	DW	01cH
	DW	01dH
	DW	01dH
	DW	040H
	DW	040H
CONST	ENDS
PUBLIC	_inflate_table
; Function compile flags: /Odtp
; File f:\mycode\hack12306\zlib-1.2.10\inftrees.c
_TEXT	SEGMENT
_offs$ = -148						; size = 32
_count$ = -116						; size = 32
tv188 = -84						; size = 4
_mask$ = -80						; size = 4
_base$ = -76						; size = 4
_extra$ = -72						; size = 4
_fill$ = -68						; size = 4
tv277 = -62						; size = 2
_low$ = -60						; size = 4
_next$ = -56						; size = 4
_match$ = -52						; size = 4
_used$ = -48						; size = 4
_incr$ = -44						; size = 4
_curr$ = -40						; size = 4
_drop$ = -36						; size = 4
_min$ = -32						; size = 4
_root$ = -28						; size = 4
_max$ = -24						; size = 4
_huff$ = -20						; size = 4
_left$ = -16						; size = 4
_here$ = -12						; size = 4
_sym$ = -8						; size = 4
_len$ = -4						; size = 4
_type$ = 8						; size = 4
_lens$ = 12						; size = 4
_codes$ = 16						; size = 4
_table$ = 20						; size = 4
_bits$ = 24						; size = 4
_work$ = 28						; size = 4
_inflate_table PROC

; 39   : {

	push	ebp
	mov	ebp, esp
	sub	esp, 148				; 00000094H

; 40   :     unsigned len;               /* a code's length in bits */
; 41   :     unsigned sym;               /* index of code symbols */
; 42   :     unsigned min, max;          /* minimum and maximum code lengths */
; 43   :     unsigned root;              /* number of index bits for root table */
; 44   :     unsigned curr;              /* number of index bits for current table */
; 45   :     unsigned drop;              /* code bits to drop for sub-table */
; 46   :     int left;                   /* number of prefix codes available */
; 47   :     unsigned used;              /* code entries in table used */
; 48   :     unsigned huff;              /* Huffman code */
; 49   :     unsigned incr;              /* for incrementing code, index */
; 50   :     unsigned fill;              /* index for replicating entries */
; 51   :     unsigned low;               /* low bits for current root entry */
; 52   :     unsigned mask;              /* mask for low root bits */
; 53   :     code here;                  /* table entry for duplication */
; 54   :     code FAR *next;             /* next available space in table */
; 55   :     const unsigned short FAR *base;     /* base value table to use */
; 56   :     const unsigned short FAR *extra;    /* extra bits table to use */
; 57   :     unsigned match;             /* use base and extra for symbol >= match */
; 58   :     unsigned short count[MAXBITS+1];    /* number of codes of each length */
; 59   :     unsigned short offs[MAXBITS+1];     /* offsets in table for each length */
; 60   :     static const unsigned short lbase[31] = { /* Length codes 257..285 base */
; 61   :         3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
; 62   :         35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
; 63   :     static const unsigned short lext[31] = { /* Length codes 257..285 extra */
; 64   :         16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18,
; 65   :         19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 192, 202};
; 66   :     static const unsigned short dbase[32] = { /* Distance codes 0..29 base */
; 67   :         1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
; 68   :         257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
; 69   :         8193, 12289, 16385, 24577, 0, 0};
; 70   :     static const unsigned short dext[32] = { /* Distance codes 0..29 extra */
; 71   :         16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
; 72   :         23, 23, 24, 24, 25, 25, 26, 26, 27, 27,
; 73   :         28, 28, 29, 29, 64, 64};
; 74   : 
; 75   :     /*
; 76   :        Process a set of code lengths to create a canonical Huffman code.  The
; 77   :        code lengths are lens[0..codes-1].  Each length corresponds to the
; 78   :        symbols 0..codes-1.  The Huffman code is generated by first sorting the
; 79   :        symbols by length from short to long, and retaining the symbol order
; 80   :        for codes with equal lengths.  Then the code starts with all zero bits
; 81   :        for the first code of the shortest length, and the codes are integer
; 82   :        increments for the same length, and zeros are appended as the length
; 83   :        increases.  For the deflate format, these bits are stored backwards
; 84   :        from their more natural integer increment ordering, and so when the
; 85   :        decoding tables are built in the large loop below, the integer codes
; 86   :        are incremented backwards.
; 87   : 
; 88   :        This routine assumes, but does not check, that all of the entries in
; 89   :        lens[] are in the range 0..MAXBITS.  The caller must assure this.
; 90   :        1..MAXBITS is interpreted as that code length.  zero means that that
; 91   :        symbol does not occur in this code.
; 92   : 
; 93   :        The codes are sorted by computing a count of codes for each length,
; 94   :        creating from that a table of starting indices for each length in the
; 95   :        sorted table, and then entering the symbols in order in the sorted
; 96   :        table.  The sorted table is work[], with that space being provided by
; 97   :        the caller.
; 98   : 
; 99   :        The length counts are used for other purposes as well, i.e. finding
; 100  :        the minimum and maximum length codes, determining if there are any
; 101  :        codes at all, checking for a valid set of lengths, and looking ahead
; 102  :        at length counts to determine sub-table sizes when building the
; 103  :        decoding tables.
; 104  :      */
; 105  : 
; 106  :     /* accumulate lengths for codes (assumes lens[] all in 0..MAXBITS) */
; 107  :     for (len = 0; len <= MAXBITS; len++)

	mov	DWORD PTR _len$[ebp], 0
	jmp	SHORT $LN62@inflate_ta
$LN61@inflate_ta:
	mov	eax, DWORD PTR _len$[ebp]
	add	eax, 1
	mov	DWORD PTR _len$[ebp], eax
$LN62@inflate_ta:
	cmp	DWORD PTR _len$[ebp], 15		; 0000000fH
	ja	SHORT $LN60@inflate_ta

; 108  :         count[len] = 0;

	xor	ecx, ecx
	mov	edx, DWORD PTR _len$[ebp]
	mov	WORD PTR _count$[ebp+edx*2], cx
	jmp	SHORT $LN61@inflate_ta
$LN60@inflate_ta:

; 109  :     for (sym = 0; sym < codes; sym++)

	mov	DWORD PTR _sym$[ebp], 0
	jmp	SHORT $LN59@inflate_ta
$LN58@inflate_ta:
	mov	eax, DWORD PTR _sym$[ebp]
	add	eax, 1
	mov	DWORD PTR _sym$[ebp], eax
$LN59@inflate_ta:
	mov	ecx, DWORD PTR _sym$[ebp]
	cmp	ecx, DWORD PTR _codes$[ebp]
	jae	SHORT $LN57@inflate_ta

; 110  :         count[lens[sym]]++;

	mov	edx, DWORD PTR _sym$[ebp]
	mov	eax, DWORD PTR _lens$[ebp]
	movzx	ecx, WORD PTR [eax+edx*2]
	mov	dx, WORD PTR _count$[ebp+ecx*2]
	add	dx, 1
	mov	eax, DWORD PTR _sym$[ebp]
	mov	ecx, DWORD PTR _lens$[ebp]
	movzx	eax, WORD PTR [ecx+eax*2]
	mov	WORD PTR _count$[ebp+eax*2], dx
	jmp	SHORT $LN58@inflate_ta
$LN57@inflate_ta:

; 111  : 
; 112  :     /* bound code lengths, force root to be within code lengths */
; 113  :     root = *bits;

	mov	ecx, DWORD PTR _bits$[ebp]
	mov	edx, DWORD PTR [ecx]
	mov	DWORD PTR _root$[ebp], edx

; 114  :     for (max = MAXBITS; max >= 1; max--)

	mov	DWORD PTR _max$[ebp], 15		; 0000000fH
	jmp	SHORT $LN56@inflate_ta
$LN55@inflate_ta:
	mov	eax, DWORD PTR _max$[ebp]
	sub	eax, 1
	mov	DWORD PTR _max$[ebp], eax
$LN56@inflate_ta:
	cmp	DWORD PTR _max$[ebp], 1
	jb	SHORT $LN54@inflate_ta

; 115  :         if (count[max] != 0) break;

	mov	ecx, DWORD PTR _max$[ebp]
	movzx	edx, WORD PTR _count$[ebp+ecx*2]
	test	edx, edx
	je	SHORT $LN53@inflate_ta
	jmp	SHORT $LN54@inflate_ta
$LN53@inflate_ta:

; 116  :     if (root > max) root = max;

	jmp	SHORT $LN55@inflate_ta
$LN54@inflate_ta:
	mov	eax, DWORD PTR _root$[ebp]
	cmp	eax, DWORD PTR _max$[ebp]
	jbe	SHORT $LN52@inflate_ta
	mov	ecx, DWORD PTR _max$[ebp]
	mov	DWORD PTR _root$[ebp], ecx
$LN52@inflate_ta:

; 117  :     if (max == 0) {                     /* no symbols to code at all */

	cmp	DWORD PTR _max$[ebp], 0
	jne	SHORT $LN51@inflate_ta

; 118  :         here.op = (unsigned char)64;    /* invalid code marker */

	mov	BYTE PTR _here$[ebp], 64		; 00000040H

; 119  :         here.bits = (unsigned char)1;

	mov	BYTE PTR _here$[ebp+1], 1

; 120  :         here.val = (unsigned short)0;

	xor	edx, edx
	mov	WORD PTR _here$[ebp+2], dx

; 121  :         *(*table)++ = here;             /* make a table to force an error */

	mov	eax, DWORD PTR _table$[ebp]
	mov	ecx, DWORD PTR [eax]
	mov	edx, DWORD PTR _here$[ebp]
	mov	DWORD PTR [ecx], edx
	mov	eax, DWORD PTR _table$[ebp]
	mov	ecx, DWORD PTR [eax]
	add	ecx, 4
	mov	edx, DWORD PTR _table$[ebp]
	mov	DWORD PTR [edx], ecx

; 122  :         *(*table)++ = here;

	mov	eax, DWORD PTR _table$[ebp]
	mov	ecx, DWORD PTR [eax]
	mov	edx, DWORD PTR _here$[ebp]
	mov	DWORD PTR [ecx], edx
	mov	eax, DWORD PTR _table$[ebp]
	mov	ecx, DWORD PTR [eax]
	add	ecx, 4
	mov	edx, DWORD PTR _table$[ebp]
	mov	DWORD PTR [edx], ecx

; 123  :         *bits = 1;

	mov	eax, DWORD PTR _bits$[ebp]
	mov	DWORD PTR [eax], 1

; 124  :         return 0;     /* no symbols, but wait for decoding to report error */

	xor	eax, eax
	jmp	$LN63@inflate_ta
$LN51@inflate_ta:

; 125  :     }
; 126  :     for (min = 1; min < max; min++)

	mov	DWORD PTR _min$[ebp], 1
	jmp	SHORT $LN50@inflate_ta
$LN49@inflate_ta:
	mov	ecx, DWORD PTR _min$[ebp]
	add	ecx, 1
	mov	DWORD PTR _min$[ebp], ecx
$LN50@inflate_ta:
	mov	edx, DWORD PTR _min$[ebp]
	cmp	edx, DWORD PTR _max$[ebp]
	jae	SHORT $LN48@inflate_ta

; 127  :         if (count[min] != 0) break;

	mov	eax, DWORD PTR _min$[ebp]
	movzx	ecx, WORD PTR _count$[ebp+eax*2]
	test	ecx, ecx
	je	SHORT $LN47@inflate_ta
	jmp	SHORT $LN48@inflate_ta
$LN47@inflate_ta:

; 128  :     if (root < min) root = min;

	jmp	SHORT $LN49@inflate_ta
$LN48@inflate_ta:
	mov	edx, DWORD PTR _root$[ebp]
	cmp	edx, DWORD PTR _min$[ebp]
	jae	SHORT $LN46@inflate_ta
	mov	eax, DWORD PTR _min$[ebp]
	mov	DWORD PTR _root$[ebp], eax
$LN46@inflate_ta:

; 129  : 
; 130  :     /* check for an over-subscribed or incomplete set of lengths */
; 131  :     left = 1;

	mov	DWORD PTR _left$[ebp], 1

; 132  :     for (len = 1; len <= MAXBITS; len++) {

	mov	DWORD PTR _len$[ebp], 1
	jmp	SHORT $LN45@inflate_ta
$LN44@inflate_ta:
	mov	ecx, DWORD PTR _len$[ebp]
	add	ecx, 1
	mov	DWORD PTR _len$[ebp], ecx
$LN45@inflate_ta:
	cmp	DWORD PTR _len$[ebp], 15		; 0000000fH
	ja	SHORT $LN43@inflate_ta

; 133  :         left <<= 1;

	mov	edx, DWORD PTR _left$[ebp]
	shl	edx, 1
	mov	DWORD PTR _left$[ebp], edx

; 134  :         left -= count[len];

	mov	eax, DWORD PTR _len$[ebp]
	movzx	ecx, WORD PTR _count$[ebp+eax*2]
	mov	edx, DWORD PTR _left$[ebp]
	sub	edx, ecx
	mov	DWORD PTR _left$[ebp], edx

; 135  :         if (left < 0) return -1;        /* over-subscribed */

	jns	SHORT $LN42@inflate_ta
	or	eax, -1
	jmp	$LN63@inflate_ta
$LN42@inflate_ta:

; 136  :     }

	jmp	SHORT $LN44@inflate_ta
$LN43@inflate_ta:

; 137  :     if (left > 0 && (type == CODES || max != 1))

	cmp	DWORD PTR _left$[ebp], 0
	jle	SHORT $LN41@inflate_ta
	cmp	DWORD PTR _type$[ebp], 0
	je	SHORT $LN40@inflate_ta
	cmp	DWORD PTR _max$[ebp], 1
	je	SHORT $LN41@inflate_ta
$LN40@inflate_ta:

; 138  :         return -1;                      /* incomplete set */

	or	eax, -1
	jmp	$LN63@inflate_ta
$LN41@inflate_ta:

; 139  : 
; 140  :     /* generate offsets into symbol table for each length for sorting */
; 141  :     offs[1] = 0;

	mov	eax, 2
	shl	eax, 0
	xor	ecx, ecx
	mov	WORD PTR _offs$[ebp+eax], cx

; 142  :     for (len = 1; len < MAXBITS; len++)

	mov	DWORD PTR _len$[ebp], 1
	jmp	SHORT $LN39@inflate_ta
$LN38@inflate_ta:
	mov	edx, DWORD PTR _len$[ebp]
	add	edx, 1
	mov	DWORD PTR _len$[ebp], edx
$LN39@inflate_ta:
	cmp	DWORD PTR _len$[ebp], 15		; 0000000fH
	jae	SHORT $LN37@inflate_ta

; 143  :         offs[len + 1] = offs[len] + count[len];

	mov	eax, DWORD PTR _len$[ebp]
	movzx	ecx, WORD PTR _offs$[ebp+eax*2]
	mov	edx, DWORD PTR _len$[ebp]
	movzx	eax, WORD PTR _count$[ebp+edx*2]
	add	ecx, eax
	mov	edx, DWORD PTR _len$[ebp]
	mov	WORD PTR _offs$[ebp+edx*2+2], cx
	jmp	SHORT $LN38@inflate_ta
$LN37@inflate_ta:

; 144  : 
; 145  :     /* sort symbols by length, by symbol order within each length */
; 146  :     for (sym = 0; sym < codes; sym++)

	mov	DWORD PTR _sym$[ebp], 0
	jmp	SHORT $LN36@inflate_ta
$LN35@inflate_ta:
	mov	eax, DWORD PTR _sym$[ebp]
	add	eax, 1
	mov	DWORD PTR _sym$[ebp], eax
$LN36@inflate_ta:
	mov	ecx, DWORD PTR _sym$[ebp]
	cmp	ecx, DWORD PTR _codes$[ebp]
	jae	SHORT $LN34@inflate_ta

; 147  :         if (lens[sym] != 0) work[offs[lens[sym]]++] = (unsigned short)sym;

	mov	edx, DWORD PTR _sym$[ebp]
	mov	eax, DWORD PTR _lens$[ebp]
	movzx	ecx, WORD PTR [eax+edx*2]
	test	ecx, ecx
	je	SHORT $LN33@inflate_ta
	mov	edx, DWORD PTR _sym$[ebp]
	mov	eax, DWORD PTR _lens$[ebp]
	movzx	ecx, WORD PTR [eax+edx*2]
	movzx	edx, WORD PTR _offs$[ebp+ecx*2]
	mov	eax, DWORD PTR _work$[ebp]
	mov	cx, WORD PTR _sym$[ebp]
	mov	WORD PTR [eax+edx*2], cx
	mov	edx, DWORD PTR _sym$[ebp]
	mov	eax, DWORD PTR _lens$[ebp]
	movzx	ecx, WORD PTR [eax+edx*2]
	mov	dx, WORD PTR _offs$[ebp+ecx*2]
	add	dx, 1
	mov	eax, DWORD PTR _sym$[ebp]
	mov	ecx, DWORD PTR _lens$[ebp]
	movzx	eax, WORD PTR [ecx+eax*2]
	mov	WORD PTR _offs$[ebp+eax*2], dx
$LN33@inflate_ta:

; 148  : 
; 149  :     /*
; 150  :        Create and fill in decoding tables.  In this loop, the table being
; 151  :        filled is at next and has curr index bits.  The code being used is huff
; 152  :        with length len.  That code is converted to an index by dropping drop
; 153  :        bits off of the bottom.  For codes where len is less than drop + curr,
; 154  :        those top drop + curr - len bits are incremented through all values to
; 155  :        fill the table with replicated entries.
; 156  : 
; 157  :        root is the number of index bits for the root table.  When len exceeds
; 158  :        root, sub-tables are created pointed to by the root entry with an index
; 159  :        of the low root bits of huff.  This is saved in low to check for when a
; 160  :        new sub-table should be started.  drop is zero when the root table is
; 161  :        being filled, and drop is root when sub-tables are being filled.
; 162  : 
; 163  :        When a new sub-table is needed, it is necessary to look ahead in the
; 164  :        code lengths to determine what size sub-table is needed.  The length
; 165  :        counts are used for this, and so count[] is decremented as codes are
; 166  :        entered in the tables.
; 167  : 
; 168  :        used keeps track of how many table entries have been allocated from the
; 169  :        provided *table space.  It is checked for LENS and DIST tables against
; 170  :        the constants ENOUGH_LENS and ENOUGH_DISTS to guard against changes in
; 171  :        the initial root table size constants.  See the comments in inftrees.h
; 172  :        for more information.
; 173  : 
; 174  :        sym increments through all symbols, and the loop terminates when
; 175  :        all codes of length max, i.e. all codes, have been processed.  This
; 176  :        routine permits incomplete codes, so another loop after this one fills
; 177  :        in the rest of the decoding tables with invalid code markers.
; 178  :      */
; 179  : 
; 180  :     /* set up for code type */
; 181  :     switch (type) {

	jmp	SHORT $LN35@inflate_ta
$LN34@inflate_ta:
	mov	ecx, DWORD PTR _type$[ebp]
	mov	DWORD PTR tv188[ebp], ecx
	cmp	DWORD PTR tv188[ebp], 0
	je	SHORT $LN30@inflate_ta
	cmp	DWORD PTR tv188[ebp], 1
	je	SHORT $LN29@inflate_ta
	jmp	SHORT $LN28@inflate_ta
$LN30@inflate_ta:

; 182  :     case CODES:
; 183  :         base = extra = work;    /* dummy value--not used */

	mov	edx, DWORD PTR _work$[ebp]
	mov	DWORD PTR _extra$[ebp], edx
	mov	eax, DWORD PTR _extra$[ebp]
	mov	DWORD PTR _base$[ebp], eax

; 184  :         match = 20;

	mov	DWORD PTR _match$[ebp], 20		; 00000014H

; 185  :         break;

	jmp	SHORT $LN31@inflate_ta
$LN29@inflate_ta:

; 186  :     case LENS:
; 187  :         base = lbase;

	mov	DWORD PTR _base$[ebp], OFFSET ?lbase@?1??inflate_table@@9@9

; 188  :         extra = lext;

	mov	DWORD PTR _extra$[ebp], OFFSET ?lext@?1??inflate_table@@9@9

; 189  :         match = 257;

	mov	DWORD PTR _match$[ebp], 257		; 00000101H

; 190  :         break;

	jmp	SHORT $LN31@inflate_ta
$LN28@inflate_ta:

; 191  :     default:    /* DISTS */
; 192  :         base = dbase;

	mov	DWORD PTR _base$[ebp], OFFSET ?dbase@?1??inflate_table@@9@9

; 193  :         extra = dext;

	mov	DWORD PTR _extra$[ebp], OFFSET ?dext@?1??inflate_table@@9@9

; 194  :         match = 0;

	mov	DWORD PTR _match$[ebp], 0
$LN31@inflate_ta:

; 195  :     }
; 196  : 
; 197  :     /* initialize state for loop */
; 198  :     huff = 0;                   /* starting code */

	mov	DWORD PTR _huff$[ebp], 0

; 199  :     sym = 0;                    /* starting code symbol */

	mov	DWORD PTR _sym$[ebp], 0

; 200  :     len = min;                  /* starting code length */

	mov	ecx, DWORD PTR _min$[ebp]
	mov	DWORD PTR _len$[ebp], ecx

; 201  :     next = *table;              /* current table to fill in */

	mov	edx, DWORD PTR _table$[ebp]
	mov	eax, DWORD PTR [edx]
	mov	DWORD PTR _next$[ebp], eax

; 202  :     curr = root;                /* current table index bits */

	mov	ecx, DWORD PTR _root$[ebp]
	mov	DWORD PTR _curr$[ebp], ecx

; 203  :     drop = 0;                   /* current bits to drop from code for index */

	mov	DWORD PTR _drop$[ebp], 0

; 204  :     low = (unsigned)(-1);       /* trigger new sub-table when len > root */

	mov	DWORD PTR _low$[ebp], -1

; 205  :     used = 1U << root;          /* use root table entries */

	mov	edx, 1
	mov	ecx, DWORD PTR _root$[ebp]
	shl	edx, cl
	mov	DWORD PTR _used$[ebp], edx

; 206  :     mask = used - 1;            /* mask for comparing low */

	mov	eax, DWORD PTR _used$[ebp]
	sub	eax, 1
	mov	DWORD PTR _mask$[ebp], eax

; 207  : 
; 208  :     /* check available table space */
; 209  :     if ((type == LENS && used > ENOUGH_LENS) ||
; 210  :         (type == DISTS && used > ENOUGH_DISTS))

	cmp	DWORD PTR _type$[ebp], 1
	jne	SHORT $LN25@inflate_ta
	cmp	DWORD PTR _used$[ebp], 852		; 00000354H
	ja	SHORT $LN26@inflate_ta
$LN25@inflate_ta:
	cmp	DWORD PTR _type$[ebp], 2
	jne	SHORT $LN24@inflate_ta
	cmp	DWORD PTR _used$[ebp], 592		; 00000250H
	jbe	SHORT $LN24@inflate_ta
$LN26@inflate_ta:

; 211  :         return 1;

	mov	eax, 1
	jmp	$LN63@inflate_ta
$LN24@inflate_ta:

; 212  : 
; 213  :     /* process all codes and make table entries */
; 214  :     for (;;) {
; 215  :         /* create table entry */
; 216  :         here.bits = (unsigned char)(len - drop);

	mov	ecx, DWORD PTR _len$[ebp]
	sub	ecx, DWORD PTR _drop$[ebp]
	mov	BYTE PTR _here$[ebp+1], cl

; 217  :         if (work[sym] + 1U < match) {

	mov	edx, DWORD PTR _sym$[ebp]
	mov	eax, DWORD PTR _work$[ebp]
	movzx	ecx, WORD PTR [eax+edx*2]
	add	ecx, 1
	cmp	ecx, DWORD PTR _match$[ebp]
	jae	SHORT $LN22@inflate_ta

; 218  :             here.op = (unsigned char)0;

	mov	BYTE PTR _here$[ebp], 0

; 219  :             here.val = work[sym];

	mov	edx, DWORD PTR _sym$[ebp]
	mov	eax, DWORD PTR _work$[ebp]
	mov	cx, WORD PTR [eax+edx*2]
	mov	WORD PTR _here$[ebp+2], cx
	jmp	SHORT $LN21@inflate_ta
$LN22@inflate_ta:

; 220  :         }
; 221  :         else if (work[sym] >= match) {

	mov	edx, DWORD PTR _sym$[ebp]
	mov	eax, DWORD PTR _work$[ebp]
	movzx	ecx, WORD PTR [eax+edx*2]
	cmp	ecx, DWORD PTR _match$[ebp]
	jb	SHORT $LN20@inflate_ta

; 222  :             here.op = (unsigned char)(extra[work[sym] - match]);

	mov	edx, DWORD PTR _sym$[ebp]
	mov	eax, DWORD PTR _work$[ebp]
	movzx	ecx, WORD PTR [eax+edx*2]
	sub	ecx, DWORD PTR _match$[ebp]
	mov	edx, DWORD PTR _extra$[ebp]
	mov	al, BYTE PTR [edx+ecx*2]
	mov	BYTE PTR _here$[ebp], al

; 223  :             here.val = base[work[sym] - match];

	mov	ecx, DWORD PTR _sym$[ebp]
	mov	edx, DWORD PTR _work$[ebp]
	movzx	eax, WORD PTR [edx+ecx*2]
	sub	eax, DWORD PTR _match$[ebp]
	mov	ecx, DWORD PTR _base$[ebp]
	mov	dx, WORD PTR [ecx+eax*2]
	mov	WORD PTR _here$[ebp+2], dx

; 224  :         }
; 225  :         else {

	jmp	SHORT $LN21@inflate_ta
$LN20@inflate_ta:

; 226  :             here.op = (unsigned char)(32 + 64);         /* end of block */

	mov	BYTE PTR _here$[ebp], 96		; 00000060H

; 227  :             here.val = 0;

	xor	eax, eax
	mov	WORD PTR _here$[ebp+2], ax
$LN21@inflate_ta:

; 228  :         }
; 229  : 
; 230  :         /* replicate for those indices with low len bits equal to huff */
; 231  :         incr = 1U << (len - drop);

	mov	ecx, DWORD PTR _len$[ebp]
	sub	ecx, DWORD PTR _drop$[ebp]
	mov	edx, 1
	shl	edx, cl
	mov	DWORD PTR _incr$[ebp], edx

; 232  :         fill = 1U << curr;

	mov	eax, 1
	mov	ecx, DWORD PTR _curr$[ebp]
	shl	eax, cl
	mov	DWORD PTR _fill$[ebp], eax

; 233  :         min = fill;                 /* save offset to next table */

	mov	ecx, DWORD PTR _fill$[ebp]
	mov	DWORD PTR _min$[ebp], ecx
$LN18@inflate_ta:

; 234  :         do {
; 235  :             fill -= incr;

	mov	edx, DWORD PTR _fill$[ebp]
	sub	edx, DWORD PTR _incr$[ebp]
	mov	DWORD PTR _fill$[ebp], edx

; 236  :             next[(huff >> drop) + fill] = here;

	mov	eax, DWORD PTR _huff$[ebp]
	mov	ecx, DWORD PTR _drop$[ebp]
	shr	eax, cl
	add	eax, DWORD PTR _fill$[ebp]
	mov	ecx, DWORD PTR _next$[ebp]
	mov	edx, DWORD PTR _here$[ebp]
	mov	DWORD PTR [ecx+eax*4], edx

; 237  :         } while (fill != 0);

	cmp	DWORD PTR _fill$[ebp], 0
	jne	SHORT $LN18@inflate_ta

; 238  : 
; 239  :         /* backwards increment the len-bit code huff */
; 240  :         incr = 1U << (len - 1);

	mov	ecx, DWORD PTR _len$[ebp]
	sub	ecx, 1
	mov	eax, 1
	shl	eax, cl
	mov	DWORD PTR _incr$[ebp], eax
$LN15@inflate_ta:

; 241  :         while (huff & incr)

	mov	ecx, DWORD PTR _huff$[ebp]
	and	ecx, DWORD PTR _incr$[ebp]
	je	SHORT $LN14@inflate_ta

; 242  :             incr >>= 1;

	mov	edx, DWORD PTR _incr$[ebp]
	shr	edx, 1
	mov	DWORD PTR _incr$[ebp], edx
	jmp	SHORT $LN15@inflate_ta
$LN14@inflate_ta:

; 243  :         if (incr != 0) {

	cmp	DWORD PTR _incr$[ebp], 0
	je	SHORT $LN13@inflate_ta

; 244  :             huff &= incr - 1;

	mov	eax, DWORD PTR _incr$[ebp]
	sub	eax, 1
	and	eax, DWORD PTR _huff$[ebp]
	mov	DWORD PTR _huff$[ebp], eax

; 245  :             huff += incr;

	mov	ecx, DWORD PTR _huff$[ebp]
	add	ecx, DWORD PTR _incr$[ebp]
	mov	DWORD PTR _huff$[ebp], ecx

; 246  :         }
; 247  :         else

	jmp	SHORT $LN12@inflate_ta
$LN13@inflate_ta:

; 248  :             huff = 0;

	mov	DWORD PTR _huff$[ebp], 0
$LN12@inflate_ta:

; 249  : 
; 250  :         /* go to next symbol, update count, len */
; 251  :         sym++;

	mov	edx, DWORD PTR _sym$[ebp]
	add	edx, 1
	mov	DWORD PTR _sym$[ebp], edx

; 252  :         if (--(count[len]) == 0) {

	mov	eax, DWORD PTR _len$[ebp]
	mov	cx, WORD PTR _count$[ebp+eax*2]
	sub	cx, 1
	mov	WORD PTR tv277[ebp], cx
	mov	edx, DWORD PTR _len$[ebp]
	mov	ax, WORD PTR tv277[ebp]
	mov	WORD PTR _count$[ebp+edx*2], ax
	movzx	ecx, WORD PTR tv277[ebp]
	test	ecx, ecx
	jne	SHORT $LN11@inflate_ta

; 253  :             if (len == max) break;

	mov	edx, DWORD PTR _len$[ebp]
	cmp	edx, DWORD PTR _max$[ebp]
	jne	SHORT $LN10@inflate_ta
	jmp	$LN23@inflate_ta
$LN10@inflate_ta:

; 254  :             len = lens[work[sym]];

	mov	eax, DWORD PTR _sym$[ebp]
	mov	ecx, DWORD PTR _work$[ebp]
	movzx	edx, WORD PTR [ecx+eax*2]
	mov	eax, DWORD PTR _lens$[ebp]
	movzx	ecx, WORD PTR [eax+edx*2]
	mov	DWORD PTR _len$[ebp], ecx
$LN11@inflate_ta:

; 255  :         }
; 256  : 
; 257  :         /* create new sub-table if needed */
; 258  :         if (len > root && (huff & mask) != low) {

	mov	edx, DWORD PTR _len$[ebp]
	cmp	edx, DWORD PTR _root$[ebp]
	jbe	$LN9@inflate_ta
	mov	eax, DWORD PTR _huff$[ebp]
	and	eax, DWORD PTR _mask$[ebp]
	cmp	eax, DWORD PTR _low$[ebp]
	je	$LN9@inflate_ta

; 259  :             /* if first time, transition to sub-tables */
; 260  :             if (drop == 0)

	cmp	DWORD PTR _drop$[ebp], 0
	jne	SHORT $LN8@inflate_ta

; 261  :                 drop = root;

	mov	ecx, DWORD PTR _root$[ebp]
	mov	DWORD PTR _drop$[ebp], ecx
$LN8@inflate_ta:

; 262  : 
; 263  :             /* increment past last table */
; 264  :             next += min;            /* here min is 1 << curr */

	mov	edx, DWORD PTR _min$[ebp]
	mov	eax, DWORD PTR _next$[ebp]
	lea	ecx, DWORD PTR [eax+edx*4]
	mov	DWORD PTR _next$[ebp], ecx

; 265  : 
; 266  :             /* determine length of next table */
; 267  :             curr = len - drop;

	mov	edx, DWORD PTR _len$[ebp]
	sub	edx, DWORD PTR _drop$[ebp]
	mov	DWORD PTR _curr$[ebp], edx

; 268  :             left = (int)(1 << curr);

	mov	eax, 1
	mov	ecx, DWORD PTR _curr$[ebp]
	shl	eax, cl
	mov	DWORD PTR _left$[ebp], eax
$LN7@inflate_ta:

; 269  :             while (curr + drop < max) {

	mov	ecx, DWORD PTR _curr$[ebp]
	add	ecx, DWORD PTR _drop$[ebp]
	cmp	ecx, DWORD PTR _max$[ebp]
	jae	SHORT $LN6@inflate_ta

; 270  :                 left -= count[curr + drop];

	mov	edx, DWORD PTR _curr$[ebp]
	add	edx, DWORD PTR _drop$[ebp]
	movzx	eax, WORD PTR _count$[ebp+edx*2]
	mov	ecx, DWORD PTR _left$[ebp]
	sub	ecx, eax
	mov	DWORD PTR _left$[ebp], ecx

; 271  :                 if (left <= 0) break;

	cmp	DWORD PTR _left$[ebp], 0
	jg	SHORT $LN5@inflate_ta
	jmp	SHORT $LN6@inflate_ta
$LN5@inflate_ta:

; 272  :                 curr++;

	mov	edx, DWORD PTR _curr$[ebp]
	add	edx, 1
	mov	DWORD PTR _curr$[ebp], edx

; 273  :                 left <<= 1;

	mov	eax, DWORD PTR _left$[ebp]
	shl	eax, 1
	mov	DWORD PTR _left$[ebp], eax

; 274  :             }

	jmp	SHORT $LN7@inflate_ta
$LN6@inflate_ta:

; 275  : 
; 276  :             /* check for enough space */
; 277  :             used += 1U << curr;

	mov	edx, 1
	mov	ecx, DWORD PTR _curr$[ebp]
	shl	edx, cl
	add	edx, DWORD PTR _used$[ebp]
	mov	DWORD PTR _used$[ebp], edx

; 278  :             if ((type == LENS && used > ENOUGH_LENS) ||
; 279  :                 (type == DISTS && used > ENOUGH_DISTS))

	cmp	DWORD PTR _type$[ebp], 1
	jne	SHORT $LN2@inflate_ta
	cmp	DWORD PTR _used$[ebp], 852		; 00000354H
	ja	SHORT $LN3@inflate_ta
$LN2@inflate_ta:
	cmp	DWORD PTR _type$[ebp], 2
	jne	SHORT $LN4@inflate_ta
	cmp	DWORD PTR _used$[ebp], 592		; 00000250H
	jbe	SHORT $LN4@inflate_ta
$LN3@inflate_ta:

; 280  :                 return 1;

	mov	eax, 1
	jmp	$LN63@inflate_ta
$LN4@inflate_ta:

; 281  : 
; 282  :             /* point entry in root table to sub-table */
; 283  :             low = huff & mask;

	mov	eax, DWORD PTR _huff$[ebp]
	and	eax, DWORD PTR _mask$[ebp]
	mov	DWORD PTR _low$[ebp], eax

; 284  :             (*table)[low].op = (unsigned char)curr;

	mov	ecx, DWORD PTR _table$[ebp]
	mov	edx, DWORD PTR [ecx]
	mov	eax, DWORD PTR _low$[ebp]
	mov	cl, BYTE PTR _curr$[ebp]
	mov	BYTE PTR [edx+eax*4], cl

; 285  :             (*table)[low].bits = (unsigned char)root;

	mov	edx, DWORD PTR _table$[ebp]
	mov	eax, DWORD PTR [edx]
	mov	ecx, DWORD PTR _low$[ebp]
	mov	dl, BYTE PTR _root$[ebp]
	mov	BYTE PTR [eax+ecx*4+1], dl

; 286  :             (*table)[low].val = (unsigned short)(next - *table);

	mov	eax, DWORD PTR _table$[ebp]
	mov	ecx, DWORD PTR _next$[ebp]
	sub	ecx, DWORD PTR [eax]
	sar	ecx, 2
	mov	edx, DWORD PTR _table$[ebp]
	mov	eax, DWORD PTR [edx]
	mov	edx, DWORD PTR _low$[ebp]
	mov	WORD PTR [eax+edx*4+2], cx
$LN9@inflate_ta:

; 287  :         }
; 288  :     }

	jmp	$LN24@inflate_ta
$LN23@inflate_ta:

; 289  : 
; 290  :     /* fill in remaining table entry if code is incomplete (guaranteed to have
; 291  :        at most one remaining entry, since if the code is incomplete, the
; 292  :        maximum code length that was allowed to get this far is one bit) */
; 293  :     if (huff != 0) {

	cmp	DWORD PTR _huff$[ebp], 0
	je	SHORT $LN1@inflate_ta

; 294  :         here.op = (unsigned char)64;            /* invalid code marker */

	mov	BYTE PTR _here$[ebp], 64		; 00000040H

; 295  :         here.bits = (unsigned char)(len - drop);

	mov	eax, DWORD PTR _len$[ebp]
	sub	eax, DWORD PTR _drop$[ebp]
	mov	BYTE PTR _here$[ebp+1], al

; 296  :         here.val = (unsigned short)0;

	xor	ecx, ecx
	mov	WORD PTR _here$[ebp+2], cx

; 297  :         next[huff] = here;

	mov	edx, DWORD PTR _huff$[ebp]
	mov	eax, DWORD PTR _next$[ebp]
	mov	ecx, DWORD PTR _here$[ebp]
	mov	DWORD PTR [eax+edx*4], ecx
$LN1@inflate_ta:

; 298  :     }
; 299  : 
; 300  :     /* set return parameters */
; 301  :     *table += used;

	mov	edx, DWORD PTR _table$[ebp]
	mov	eax, DWORD PTR [edx]
	mov	ecx, DWORD PTR _used$[ebp]
	lea	edx, DWORD PTR [eax+ecx*4]
	mov	eax, DWORD PTR _table$[ebp]
	mov	DWORD PTR [eax], edx

; 302  :     *bits = root;

	mov	ecx, DWORD PTR _bits$[ebp]
	mov	edx, DWORD PTR _root$[ebp]
	mov	DWORD PTR [ecx], edx

; 303  :     return 0;

	xor	eax, eax
$LN63@inflate_ta:

; 304  : }

	mov	esp, ebp
	pop	ebp
	ret	0
_inflate_table ENDP
_TEXT	ENDS
END
