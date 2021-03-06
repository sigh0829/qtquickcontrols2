import QtQuick 2.0
import QmlBench 1.0
import QtQuick.Controls 2.0

CreationBenchmark {
    id: root
    count: 20
    staticCount: 2000
    delegate: Pane {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        Item {
            implicitWidth: 100
            implicitHeight: 100
        }
    }
}
