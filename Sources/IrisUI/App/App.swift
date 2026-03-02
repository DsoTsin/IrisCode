import _IrisUI

public typealias CGPoint = iris.ui.point
public typealias CGRect = iris.ui.rect

public protocol App {
    
    @MainActor
    @preconcurrency
    init()
}

extension App {

    @MainActor
    @preconcurrency
    public static func main() {
        let app = Self()
        //runApp(app)
    }
}

struct Window: iris.ui.view {

}
