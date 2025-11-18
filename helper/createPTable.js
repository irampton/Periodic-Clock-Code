const fs = require('fs');
const path = require('path');
const pTable = require("./Periodic_Table.json");

const elementFamily = Object.fromEntries(
    [...new Set(pTable.map(e => e.Details["Metal-Type"]))]
        .map((name, i) => [name, i + 1])
);

const orderedColumns = [
    "Hydrogen",
    "Alkali metals",
    "Alkaline earth metals",
    "Boron family",
    "Carbon family",
    "Nitrogen family",
    "Oxygen family",
    "Halogens",
    "Noble gases",
    "Transition metals",
    "Lanthanoids",
    "Actinoids",
    "N/A",
];

const elementColumn = Object.fromEntries(orderedColumns.map((name, i) => [name, i + 1]));

function getColumnName(element) {
    const metalType = element.Details["Metal-Type"];
    const atomicNumber = element["Atomic-Number"];

    if (atomicNumber === 1) {
        return "Hydrogen";
    }

    if ([
        "Noble gases",
        "Alkali metals",
        "Alkaline earth metals",
        "Transition metals",
        "Lanthanoids",
        "Actinoids",
        "N/A",
    ].includes(metalType)) {
        return metalType;
    }

    const valence = element.Details["Energy-Levels"].slice(-1)[0];

    switch (valence) {
        case 3:
            return "Boron family";
        case 4:
            return "Carbon family";
        case 5:
            return "Nitrogen family";
        case 6:
            return "Oxygen family";
        case 7:
            return "Halogens";
        default:
            return "N/A";
    }
}

const cOut = `// This is an automatically generated .cpp file
#include "Periodic_Conversion.h"

// Metal types:
${Object.keys(elementFamily).map((t, i) => `// ${(i + 1).toString().padStart(2, '0')}:   ${t}`).join("\n")}

// Columns:
${orderedColumns.map((t, i) => `// ${(i + 1).toString().padStart(2, '0')}:   ${t}`).join("\n")}

const std::string pTable[${pTable.length + 1}] = {"00", ${pTable.map((e, i) => `${i % 10 ? " " : "\n"}"${e.Symbol}"`).toString()}
};

const uint8_t elementFamily[${pTable.length + 1}] = {0, ${pTable.map((e, i) => `${i % 10 ? " " : "\n"}${elementFamily[e.Details["Metal-Type"]]}`).toString()}
};

const uint8_t elementColumn[${pTable.length + 1}] = {0, ${pTable.map((e, i) => `${i % 10 ? " " : "\n"}${elementColumn[getColumnName(e)]}`).toString()}
};
`;


fs.writeFileSync(path.join(__dirname, '..', 'src', 'Periodic_Conversion.cpp'), cOut, 'utf8');
