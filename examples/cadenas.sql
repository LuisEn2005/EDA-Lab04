SELECT 
    lexema,
    CASE 
        WHEN LOWER(lexema) = 'select' THEN 'KEYWORD_SELECT'
        WHEN LOWER(lexema) = 'from'   THEN 'KEYWORD_FROM'
        WHEN LOWER(lexema) = 'where'  THEN 'KEYWORD_WHERE'
        ELSE 'IDENTIFIER'
    END AS token_tipo
FROM regexp_split_to_table(
    'SELECT select SeLeCt selected from_table WHERE_clause', 
    '\s+'
) AS lexema
WHERE lexema <> '';