SELECT nombre, edad, carrera
FROM estudiantes
WHERE edad >= 18 AND carrera = 'CC' OR promedio <> 10.5 AND codigo != 7;
