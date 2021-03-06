//
//  LineBlock.h
//  iTerm
//
//  Created by George Nachman on 11/21/13.
//
//

#import <Foundation/Foundation.h>
#import "ScreenChar.h"

// LineBlock represents an ordered collection of lines of text. It stores them contiguously
// in a buffer.
@interface LineBlock : NSObject {
    // The raw lines, end-to-end. There is no delimiter between each line.
    screen_char_t* raw_buffer;
    screen_char_t* buffer_start;  // usable start of buffer (stuff before this is dropped)
    
    int start_offset;  // distance from raw_buffer to buffer_start
    int first_entry;  // first valid cumulative_line_length
    
    // The number of elements allocated for raw_buffer.
    int buffer_size;
    
    // There will be as many entries in this array as there are lines in raw_buffer.
    // The ith value is the length of the ith line plus the value of
    // cumulative_line_lengths[i-1] for i>0 or 0 for i==0.
    int* cumulative_line_lengths;
    NSTimeInterval *timestamps_;
    
    // The number of elements allocated for cumulative_line_lengths.
    int cll_capacity;
    
    // The number of values in the cumulative_line_lengths array.
    int cll_entries;
    
    // If true, then the last raw line does not include a logical newline at its terminus.
    BOOL is_partial;
    
    // The number of wrapped lines if width==cached_numlines_width.
    int cached_numlines;
    
    // This is -1 if the cache is invalid; otherwise it specifies the width for which
    // cached_numlines is correct.
    int cached_numlines_width;
}

- (LineBlock*) initWithRawBufferSize: (int) size;
- (LineBlock *)copy;

- (void) dealloc;

// Try to append a line to the end of the buffer. Returns false if it does not fit. If length > buffer_size it will never succeed.
// Callers should split such lines into multiple pieces.
- (BOOL)appendLine:(screen_char_t*)buffer
            length:(int)length
           partial:(BOOL)partial
             width:(int)width
         timestamp:(NSTimeInterval)timestamp;

// Try to get a line that is lineNum after the first line in this block after wrapping them to a given width.
// If the line is present, return a pointer to its start and fill in *lineLength with the number of bytes in the line.
// If the line is not present, decrement *lineNum by the number of lines in this block and return NULL.
- (screen_char_t*)getWrappedLineWithWrapWidth:(int)width
                                      lineNum:(int*)lineNum
                                   lineLength:(int*)lineLength
                            includesEndOfLine:(int*)includesEndOfLine;

// Sets *yOffsetPtr (if not null) to the number of consecutive empty lines just before |lineNum| because
// there's no way for the returned pointer to indicate this.
- (screen_char_t*)getWrappedLineWithWrapWidth:(int)width
                                      lineNum:(int*)lineNum
                                   lineLength:(int*)lineLength
                            includesEndOfLine:(int*)includesEndOfLine
                                      yOffset:(int*)yOffsetPtr;

// Get the number of lines in this block at a given screen width.
- (int)getNumLinesWithWrapWidth: (int) width;

// Returns whether getNumLinesWithWrapWidth will be fast.
- (BOOL)hasCachedNumLinesForWidth: (int) width;

// Returns true if the last line is incomplete.
- (BOOL)hasPartial;

// Remove the last line. Returns false if there was none.
- (BOOL)popLastLineInto:(screen_char_t**)ptr
             withLength:(int*)length
              upToWidth:(int)width
              timestamp:(NSTimeInterval *)timestampPtr;

// Drop lines from the start of the buffer. Returns the number of lines actually dropped
// (either n or the number of lines in the block).
- (int)dropLines:(int)n withWidth:(int)width chars:(int *)charsDropped;

// Returns true if there are no lines in the block
- (BOOL)isEmpty;

// Grow the buffer.
- (void)changeBufferSize: (int) capacity;

// Get the size of the raw buffer.
- (int)rawBufferSize;

// Return the number of raw (unwrapped) lines
- (int)numRawLines;

// Return the position of the first used character in the raw buffer. Only valid if not empty.
- (int)startOffset;

// Return the length of a raw (unwrapped) line
- (int)getRawLineLength: (int) linenum;

// Remove extra space from the end of the buffer. Future appends will fail.
- (void)shrinkToFit;

// Append a value to cumulativeLineLengths.
- (void)_appendCumulativeLineLength:(int)cumulativeLength
                          timestamp:(NSTimeInterval)timestamp;

// Return a raw line
- (screen_char_t *)rawLine: (int) linenum;

// NSLog the contents of the block. For debugging.
- (void)dump:(int)rawOffset;

// Returns the timestamp associated with a line when wrapped to the specified width.
- (NSTimeInterval)timestampForLineNumber:(int)lineNum width:(int)width;

// Appends the contents of the block to |s|.
- (void)appendToDebugString:(NSMutableString *)s;

// Returns the total number of bytes used, including dropped chars.
- (int)rawSpaceUsed;

// Returns the total number of lines, including dropped lines.
- (int)numEntries;

// Searches for a substring, populating results with ResultRange objects.
- (void)findSubstring:(NSString*)substring
              options:(int)options
             atOffset:(int)offset
              results:(NSMutableArray*)results
      multipleResults:(BOOL)multipleResults;

// Tries to convert a byte offset into the block to an x,y coordinate relative to the first char
// in the block. Returns YES on success, NO if the position is out of range.
- (BOOL)convertPosition:(int)position
              withWidth:(int)width
                    toX:(int*)x
                    toY:(int*)y;

// Returns the position of a char at (x, lineNum). Fills in yOffsetPtr with number of blank lines
// before that cell, and sets *extendsPtr if x is at the right margin (after nulls).
- (int)getPositionOfLine:(int*)lineNum
                     atX:(int)x
               withWidth:(int)width
                 yOffset:(int *)yOffsetPtr
                 extends:(BOOL *)extendsPtr;

// Count the number of "full lines" in buffer up to position 'length'. A full
// line is one that, after wrapping, goes all the way to the edge of the screen
// and has at least one character wrap around. It is equal to the number of
// lines after wrapping minus one. Examples:
//
// 2 Full Lines:    0 Full Lines:   0 Full Lines:    1 Full Line:
// |xxxxx|          |x     |        |xxxxxx|         |xxxxxx|
// |xxxxx|                                           |x     |
// |x    |
int NumberOfFullLines(screen_char_t* buffer, int length, int width);


// Finds a where the nth line begins after wrapping and returns its offset from the start of the buffer.
//
// In the following example, this would return:
// pointer to a if n==0, pointer to g if n==1, asserts if n > 1
// |abcdef|
// |ghi   |
//
// It's more complex with double-width characters.
// In this example, suppose XX is a double-width character.
//
// Returns a pointer to a if n==0, pointer XX if n==1, asserts if n > 1:
// |abcde|   <- line is short after wrapping
// |XXzzzz|
int OffsetOfWrappedLine(screen_char_t* p, int n, int length, int width);

@end
