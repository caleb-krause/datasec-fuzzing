#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <assert.h>
#include "miniply.h"

#pragma clang optimize off
#pragma GCC optimize("O0")

const bool debug = false;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "no file name :(\n");
        exit(EXIT_FAILURE);
    }

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#endif

    while (__AFL_LOOP(10000))
    {
        miniply::PLYReader reader(argv[1]);
        if (!reader.valid())
        {
            fprintf(stderr, "Failed to open %s\n", argv[1]);
            continue;
        }

        // Read header metadata
        reader.file_type();
        reader.version_major();
        reader.version_minor();
        reader.num_elements();

        for (; reader.has_element(); reader.next_element())
        {
            if (!reader.load_element())
                continue;

            const miniply::PLYElement* elem = reader.element();
            if (!elem)
                continue;

            uint32_t nRows  = reader.num_rows();
            if (debug) printf("%d rows\n", nRows);
            uint32_t nProps = (uint32_t)elem->properties.size();
            if (debug) printf("%d properties\n", nProps);

            // Iterate over every property as declared in the header.
            // No assumptions about count or type — we read what's there.
            for (uint32_t p = 0; p < nProps; ++p)
            {
                const miniply::PLYProperty& prop = elem->properties[p];

                if (prop.countType == miniply::PLYPropertyType::None)
                {
                    // Scalar property: one value per row
                    std::vector<float> buf(nRows);
                    reader.extract_properties(&p, 1,
                        miniply::PLYPropertyType::Float, buf.data());
                }
                else
                {
                    // List property: variable number of values per row.
                    // prop.rowCount[i] gives the count for row i.
                    // Sum them to know the total number of values.

                    assert(prop.rowCount.size() == nRows); // should hold if miniply is correct

                    uint32_t total = 0;
                    for (uint32_t i = 0; i < nRows; ++i)
                        total += prop.rowCount[i];

                    if (total > 0)
                    {
                        std::vector<float> buf(total);
                        reader.extract_list_property(p,
                            miniply::PLYPropertyType::Float, buf.data());
                    }
                }
            }
        }
    }
}