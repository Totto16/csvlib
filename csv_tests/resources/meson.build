resource_files = files(
    'classification.csv',
    'ford_escort.csv',
    'korean-small.csv',
    'korean.csv',
    'orig.csv',
    'simple_csv_utf8_bom.tsv',
    'small_sample.tsv',
    'small_sample_equal_not_equal_bom.tsv',
)

fs = import('fs')

foreach file : resource_files
    fs.copyfile(file)
endforeach
