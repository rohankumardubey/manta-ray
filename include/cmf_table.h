#ifndef MANTARAY_CMF_TABLE_H
#define MANTARAY_CMF_TABLE_H

#include "manta_math.h"
#include "color.h"

namespace manta {

    // Forward declarations
    class Spectrum;

    class CmfTable {
    public:
        struct Entry {
            math::real_d wavelength;
            ColorXyz colorXyz;
        };

    public:
        CmfTable();
        ~CmfTable();

        bool loadCsv(const char *fname);
        void destroy();

        math::Vector pureToRgb(math::real_d wavelength) const;
        math::Vector xyzToRgb(const ColorXyz &color) const;
        Entry sampleXyz(math::real_d wavelength) const;

        ColorXyz spectralToXyz(const Spectrum *spectrum) const;

        int getEntryCount() const { return m_entryCount; }

    protected:
        Entry *m_entries;
        int m_entryCount;
    };

} /* namespace manta */

#endif /* MANTARAY_CMF_TABLE_H */
