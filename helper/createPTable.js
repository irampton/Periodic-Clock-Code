const fs = require('fs');
const pTable = require("./Periodic_Table.json");

const types = Object.fromEntries(
    [...new Set(pTable.map(e => e.Details["Metal-Type"]))]
        .map((name, i) => [name, i + 1])
);

const cOut = `// This is an automatically genrated .cpp file
#include "Periodic_Conversion.h"

// Types:
${Object.keys(types).map((t, i) => `// ${(i + 1).toString().padStart(2, '0')}:   ${t}`).join("\n")}

const std::string pTable[${pTable.length + 1}] = {"  ", ${pTable.map((e, i) => `${i % 10 ? " " : "\n"}"${e.Symbol}"`).toString()}
};

const uint8_t pTableColor[${pTable.length + 1}] = {0, ${pTable.map((e, i) => `${i % 10 ? " " : "\n"}${types[e.Details["Metal-Type"]]}`).toString()}
};
`;


fs.writeFileSync('../src/Periodic_Conversion.cpp', cOut, 'utf8');