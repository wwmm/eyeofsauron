pragma ComponentBehavior: Bound
pragma Singleton
import QtQuick

QtObject {
    function isEmpty(v: string): bool {
        if (v === undefined || v === null) {
            return true;
        }

        return v.length === 0;
    }
}
