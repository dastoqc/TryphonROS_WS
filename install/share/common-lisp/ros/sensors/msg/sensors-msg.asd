
(cl:in-package :asdf)

(defsystem "sensors-msg"
  :depends-on (:roslisp-msg-protocol :roslisp-utils )
  :components ((:file "_package")
    (:file "imuros" :depends-on ("_package_imuros"))
    (:file "_package_imuros" :depends-on ("_package"))
    (:file "sonarArray" :depends-on ("_package_sonarArray"))
    (:file "_package_sonarArray" :depends-on ("_package"))
    (:file "sonar" :depends-on ("_package_sonar"))
    (:file "_package_sonar" :depends-on ("_package"))
    (:file "forces" :depends-on ("_package_forces"))
    (:file "_package_forces" :depends-on ("_package"))
    (:file "compass" :depends-on ("_package_compass"))
    (:file "_package_compass" :depends-on ("_package"))
    (:file "motorArray" :depends-on ("_package_motorArray"))
    (:file "_package_motorArray" :depends-on ("_package"))
    (:file "motor" :depends-on ("_package_motor"))
    (:file "_package_motor" :depends-on ("_package"))
  ))